//
// Created by kh on 3/25/20.
//

#include "ProgramRunnerMt.h"

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include <boost/container/flat_set.hpp>
#include <boost/scope_exit.hpp>

namespace igen {

static bool has_char(const str &s, char c) {
    for (char x : s) if (x == c) return true;
    return false;
}

ProgramRunnerMt::ProgramRunnerMt(PMutContext _ctx) : Object(move(_ctx)) {
    timer_.stop();
    UniqueLock run_lock(run_lock_);
    WriteLock w_stat_lock(stat_lock_);

    n_threads_ = ctx()->get_option_as<int>("runner-threads");
    CHECK_GE(n_threads_, 1);

    str cache_ctl = ctx()->get_option_as<str>("cache");
    has_cache = !cache_ctl.empty();
    if (has_cache) {
        allow_cache_read = has_cache && has_char(cache_ctl, 'r');
        allow_cache_write = has_cache && has_char(cache_ctl, 'w');
        allow_execute = has_char(cache_ctl, 'x');
        CHECKF(allow_execute || (has_cache && allow_cache_read), "Invalid argument for cache control: ", cache_ctl);
    }

    if (has_cache) {
        str cachedir = ctx()->get_option_as<str>("cache-path");
        if (cachedir.empty()) cachedir = ctx()->get_option_as<str>("filestem") + ".cachedb";

        using namespace rocksdb;
        Options options;
        // Optimizes total bytes written to disk vs. logical database size (write amplification)
        options.OptimizeUniversalStyleCompaction();
        // create the DB if it's not already present
        options.create_if_missing = true;
        options.keep_log_file_num = options.recycle_log_file_num = 2;
        options.bottommost_compression = options.compression = CompressionType::kZSTD;

        DB *db;
        if (allow_cache_write) {
            Status s = DB::Open(options, cachedir, &db);
            CHECKF(s.ok(), "Fail to open cachedb (r/w) {}, at {}", s.ToString(), cachedir);
        } else {
            Status s = DB::OpenForReadOnly(options, cachedir, &db);
            CHECKF(s.ok(), "Fail to open cachedb (read only) {}, at {}", s.ToString(), cachedir);
        }
        cachedb_.reset(db);

        if (allow_cache_read) {
            cachedb_readopts_ = std::make_unique<ReadOptions>();
        }
        if (allow_cache_write) {
            cachedb_writeopts_ = std::make_unique<WriteOptions>();
            cachedb_writeopts_->disableWAL = true;
        }
    }

    if (allow_execute) {
        runners_.resize(n_threads_);
        for (int i = 0; i < n_threads_; ++i) {
            map<str, str> vars;
            vars["tid"] = fmt::format("{:0>2}", i);
            runners_[i] = new ProgramRunner(ctx_mut(), move(vars));
        }
    }
}

static inline rocksdb::Slice to_key(const PConfig &config) {
    const auto &vals = config->values();
    return rocksdb::Slice(reinterpret_cast<const char *>(vals.data()), sizeof(vals[0]) * vals.size());
}

vec<set<str>> ProgramRunnerMt::run(const vec<PMutConfig> &v_configs) {
    ReadLock r_run_entrance_lock(run_entrance_lock_);

    using namespace rocksdb;
    {
        WriteLock w_stat_lock(stat_lock_);
        CHECK_GE(n_started_timer, 0);
        if (++n_started_timer == 1) timer_.resume();
    }
    BOOST_SCOPE_EXIT(this_) {
            WriteLock w_stat_lock(this_->stat_lock_);
            CHECK_GE(this_->n_started_timer, 1);
            if (--this_->n_started_timer == 0) this_->timer_.stop();
        }
    BOOST_SCOPE_EXIT_END

    // ====
    vec<set<str>> v_locs(v_configs.size());
    vec<int> cid_to_run;
    cid_to_run.reserve(v_configs.size());

    for (int cid = 0; cid < sz(v_configs); ++cid) {
        const auto &config = v_configs[cid];
        auto &locs = v_locs[cid];

        if (allow_cache_read) {
            PinnableSlice val;
            Status s = cachedb_->Get(*cachedb_readopts_, cachedb_->DefaultColumnFamily(), to_key(config), &val);
            CHECKF(s.ok() || s.IsNotFound(), "Fail to read cache for: ") << *config;
            if (s.ok()) {
                n_cache_hit_++;
                str loc;
                for (int i = 0; i < (int) val.size(); ++i) {
                    char c = val[i];
                    if (c == ',') {
                        locs.emplace_hint(locs.end(), move(loc));
                        loc.clear();
                    } else {
                        loc.push_back(c);
                    }
                }
                continue;
            }
        }

        CHECKF(allow_execute, "Not allow_execute, config not found: ") << *config;
        cid_to_run.push_back(cid);
    }

    {
        CHECK(allow_execute || cid_to_run.empty());
        if (allow_execute && n_threads_ > 1) {
            work_queue_.run_batch_job([&v_locs,
                                              &v_configs = std::as_const(v_configs),
                                              &cid_to_run = std::as_const(cid_to_run),
                                              &runners = std::as_const(this->runners_)]
                                              (int thread_id, int item_id) {
                int cid = cid_to_run.at(item_id);
                const auto &config = v_configs.at(cid);
                auto &locs = v_locs.at(cid);
                auto &runner = runners.at(thread_id);
                locs = runner->run(config);
            }, sz(cid_to_run));
        } else if (allow_execute) {
            UniqueLock run_lock(run_lock_);

            auto &runner = runners_.at(0);
            for (int cid : cid_to_run) {
                const auto &config = v_configs.at(cid);
                auto &locs = v_locs.at(cid);
                locs = runner->run(config);
            }
        }
    }

    if (allow_cache_write) {
        for (int cid : cid_to_run) {
            const auto &config = v_configs[cid];
            const auto &locs = v_locs[cid];

            n_unflushed_write_++;
            str val;
            val.reserve(locs.size() * 32);
            for (const str &s : locs) val.append(s), val.push_back(',');
            Status s = cachedb_->Put(*cachedb_writeopts_, to_key(config), val);
            CHECKF(s.ok(), "Fail to write cache for: ") << *config;
        }
    }

    return v_locs;
}

void ProgramRunnerMt::cleanup() {
    {
        WriteLock w_run_entrance_lock(run_entrance_lock_);
        UniqueLock run_lock(run_lock_);
        WriteLock w_stat_lock(stat_lock_);

        work_queue_.stop();
    }
    flush_cachedb(), cachedb_.reset();
}

void ProgramRunnerMt::flush_cachedb() {
    WriteLock w_run_entrance_lock(run_entrance_lock_);
    UniqueLock run_lock(run_lock_);
    WriteLock w_stat_lock(stat_lock_);

    if (has_cache && n_unflushed_write_ > 0) {
        using namespace rocksdb;
        LOG(INFO, "Start flushing cache db ({} unflushed writes, {} hits)", n_unflushed_write_, n_cache_hit_);
        Status s = cachedb_->Flush({});
        CHECKF(s.ok(), "Fail to flush cachedb");
        //s = cachedb_->CompactRange(CompactRangeOptions{}, nullptr, nullptr);
        //CHECKF(s.ok(), "Fail to compact cachedb");
        LOG(INFO, "Done flushing cache db");

        n_unflushed_write_ = 0;
    }
}

void ProgramRunnerMt::init() {
    UniqueLock run_lock(run_lock_);
    WriteLock w_stat_lock(stat_lock_);

    if (allow_execute) {
        for (const auto &r: runners_) r->init();
        if (n_threads_ > 1)
            work_queue_.init(n_threads_), work_queue_.start();
        LOG(INFO, "Inited {} runners", n_threads_);
    }
}

void ProgramRunnerMt::reset_stat() {
    WriteLock w_run_entrance_lock(run_entrance_lock_);
    UniqueLock run_lock(run_lock_);
    WriteLock w_stat_lock(stat_lock_);

    n_cache_hit_ = n_started_timer = 0;
    timer_.start(), timer_.stop();
    for (const auto &r: runners_) r->reset_stat();
}

int ProgramRunnerMt::n_runs() const {
    ReadLock r_stat_lock(stat_lock_);
    int sum = 0;
    for (const auto &r: runners_) sum += r->n_runs();
    return sum;
}

int ProgramRunnerMt::n_locs() const {
    ReadLock r_stat_lock(stat_lock_);
    int ret = -1;
    for (const auto &r: runners_) {
        int n = r->n_locs();
        CHECK(n == -1 || (ret == -1 || ret == n));
        ret = n;
    }
    return ret;
}

boost::timer::cpu_times ProgramRunnerMt::total_elapsed() const {
    ReadLock r_stat_lock(stat_lock_);
    const auto t = timer_.elapsed();
    boost::timer::cpu_times sum{0, t.user, t.system};
    for (const auto &r: runners_) {
        auto elap = r->timer().elapsed();
        sum.wall += elap.wall;
    }
    return sum;
}

boost::timer::cpu_times ProgramRunnerMt::timer() const {
    ReadLock r_stat_lock(stat_lock_);
    return timer_.elapsed();
}

int ProgramRunnerMt::n_cache_hit() const {
    return n_cache_hit_; // atomic
}

void intrusive_ptr_release(const ProgramRunnerMt *p) { boost::sp_adl_block::intrusive_ptr_release(p); }

void intrusive_ptr_add_ref(const ProgramRunnerMt *p) { boost::sp_adl_block::intrusive_ptr_add_ref(p); }

}