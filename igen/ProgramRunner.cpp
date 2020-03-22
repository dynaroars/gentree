//
// Created by KH on 3/7/2020.
//

#include "ProgramRunner.h"

#include <klib/print_stl.h>

#ifdef __MINGW32__
typedef unsigned long DWORD;
#   ifndef __kernel_entry
#       define __kernel_entry
#   endif
#endif

#include <boost/process/child.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/process/args.hpp>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include "GCovRunner.h"

namespace bp = boost::process;

namespace igen {


ProgramRunner::ProgramRunner(PMutContext _ctx) : Object(move(_ctx)), type(RunnerType::Invalid) {
    if (ctx()->has_option("simple-runner")) {
        type = RunnerType::Simple;
    } else if (ctx()->has_option("builtin-runner")) {
        type = RunnerType::BuiltIn;
    } else if (ctx()->has_option("gcov-runner")) {
        type = RunnerType::GCov;
    } else {
        str str_type = ctx()->get_option_as<str>("runner");
        type = RunnerType::_from_string_nocase(str_type.c_str());
    }
    if (ctx()->has_option("target")) {
        target = ctx()->get_option_as<str>("target");
    } else {
        if (type == +RunnerType::GCov)
            target = ctx()->get_option_as<str>("filestem") + ".gcov";
        else
            target = ctx()->get_option_as<str>("filestem") + ".exe";
    }
    LOG(INFO, "Program runner: type {}, target {}", type._to_string(), target);
    if (type == +RunnerType::BuiltIn) {
        builtin_fn = builtin::get_fn(target);
        LOG(INFO, "Builtin runner source: ") << builtin::get_src(target);
    }
    if (type == +RunnerType::GCov) {
        gcov_runner_ = new GCovRunner(ctx_mut());
        gcov_runner_->parse(target);
    }

    if (ctx()->has_option("cache")) {
        str cachedir = ctx()->get_option_as<str>("cache");
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
        Status s = DB::Open(options, cachedir, &db);
        cachedb_.reset(db);
        CHECKF(s.ok(), "Fail to open cachedb at: {}", cachedir);
        cachedb_readopts_ = std::make_unique<ReadOptions>();
        cachedb_writeopts_ = std::make_unique<WriteOptions>();
        cachedb_writeopts_->disableWAL = true;
    }
}

static inline rocksdb::Slice to_key(const PConfig &config) {
    const auto &vals = config->values();
    return rocksdb::Slice(reinterpret_cast<const char *>(vals.data()), sizeof(vals[0]) * vals.size());
}

set<str> ProgramRunner::run(const PConfig &config) {
    using namespace rocksdb;

    set<str> ret;
    n_runs_++;
    if (cachedb_ != nullptr) {
        PinnableSlice val;
        Status s = cachedb_->Get(*cachedb_readopts_, cachedb_->DefaultColumnFamily(), to_key(config), &val);
        CHECKF(s.ok() || s.IsNotFound(), "Fail to read cache for: ") << *config;
        if (s.ok()) {
            n_cache_hit_++;
            str loc;
            for (int i = 0; i < (int) val.size(); ++i) {
                char c = val[i];
                if (c == ',') {
                    ret.emplace_hint(ret.end(), move(loc));
                    loc.clear();
                } else {
                    loc.push_back(c);
                }
            }
            return ret;
        }
    }
    switch (type) {
        case RunnerType::Simple:
            ret = _run_simple(config);
            break;
        case RunnerType::BuiltIn:
            ret = _run_builtin(config);
            break;
        case RunnerType::GCov:
            ret = _run_gcov(config);
            break;
        default:
            CHECK(0);
    }
    if (cachedb_ != nullptr) {
        n_cache_write_++;
        str val;
        val.reserve(ret.size() * 32);
        for (const str &s : ret) val.append(s), val.push_back(',');
        Status s = cachedb_->Put(*cachedb_writeopts_, to_key(config), val);
        CHECKF(s.ok(), "Fail to write cache for: ") << *config;
    }
    return ret;
}

set<str> ProgramRunner::_run_simple(const PConfig &config) const {
    bp::ipstream out, err;
    vec<str> args = config->value_labels();
    bp::child proc_child(target, bp::args(args), bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running process {}, args {}", target, args);

    set<str> locations;
    str loc;
    while (getline(out, loc)) {
        if (loc.size() && loc.back() == '\r')
            loc.pop_back();
        locations.insert(loc);
    }

    std::string serr(std::istreambuf_iterator<char>(err), {});
    if (serr.size()) {
        LOG(WARNING, "cerr is not empty: ") << err.rdbuf();
    }

    return locations;
}

set<str> ProgramRunner::_run_builtin(const PConfig &config) const {
    return builtin_fn(config);
}

set<str> ProgramRunner::_run_gcov(const PConfig &config) const {
    vec<str> str_args;
    str_args.reserve(size_t(dom()->n_vars()) * 2);
    for (const auto &e : *config) {
        bool on = e.label() == "on";
        bool off = e.label() == "off";
        if (on || off) {
            if (on) str_args.emplace_back(e.name());
        } else {
            if (e.name() == "+") {
                str_args.emplace_back(e.name() + e.label());
            } else if (e.name().size() > 2 && e.name()[0] == '-' && e.name()[1] == '-') {
                str_args.emplace_back(e.name() + '=' + e.label());
            } else {
                str_args.emplace_back(e.name());
                str_args.emplace_back(e.label());
            }
        }
    }
    gcov_runner_->clean_cov();
    gcov_runner_->exec(str_args);
    return gcov_runner_->collect_cov();
}

void ProgramRunner::cleanup() {
    gcov_runner_.reset();
    if (cachedb_ != nullptr && n_cache_write_ > 0) {
        using namespace rocksdb;
        LOG(INFO, "Start compacting cache db ({} writes, {} hits)", n_cache_write_, n_cache_hit_);
        Status s = cachedb_->CompactRange(CompactRangeOptions{}, nullptr, nullptr);
        CHECKF(s.ok(), "Fail to compact cachedb");
        LOG(INFO, "Done compacting cache db");
    }
    cachedb_.reset();
}

void ProgramRunner::init() {
    if (gcov_runner_ != nullptr) gcov_runner_->init();
}

int ProgramRunner::n_locs() const {
    if (gcov_runner_ != nullptr) return gcov_runner_->n_locs();
    return -1;
}

void intrusive_ptr_release(const ProgramRunner *p) { boost::sp_adl_block::intrusive_ptr_release(p); }

void intrusive_ptr_add_ref(const ProgramRunner *p) { boost::sp_adl_block::intrusive_ptr_add_ref(p); }

}