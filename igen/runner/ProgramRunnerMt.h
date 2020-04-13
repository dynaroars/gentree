//
// Created by kh on 3/25/20.
//

#ifndef IGEN4_PROGRAMRUNNERMT_H
#define IGEN4_PROGRAMRUNNERMT_H

#include <igen/Context.h>
#include <igen/Config.h>
#include <igen/runner/ProgramRunner.h>

#include <klib/WorkQueue.h>

#include <boost/timer/timer.hpp>
#include <boost/thread/tss.hpp>

#include <shared_mutex>

namespace rocksdb {
class DB;

class ReadOptions;

class WriteOptions;
}

namespace igen {

class ProgramRunnerMt : public Object {
public:
    explicit ProgramRunnerMt(PMutContext ctx);

    void init();

    vec<set<str>> run(const vec<PMutConfig> &v_configs);

    void reset_stat();

    void reset_local_timer();

    int n_runs() const;

    int n_cache_hit() const;

    void cleanup() override;

    void flush_cachedb();

    int n_locs() const;

    boost::timer::cpu_times total_elapsed() const;

    boost::timer::cpu_times timer() const;

    boost::timer::cpu_times local_timer() const;

private:
    int n_threads_ = 1;
    std::atomic<int> n_cache_hit_ = 0, n_unflushed_write_ = 0;
    bool has_cache{}, allow_cache_read{}, allow_cache_write{}, allow_execute{};

    std::unique_ptr<rocksdb::DB> cachedb_;
    std::unique_ptr<rocksdb::ReadOptions> cachedb_readopts_{};
    std::unique_ptr<rocksdb::WriteOptions> cachedb_writeopts_{};

    vec<PMutProgramRunner> runners_;
    WorkQueue work_queue_;
    boost::timer::cpu_timer timer_;
    boost::thread_specific_ptr<boost::timer::cpu_timer> local_timer_;
    int n_started_timer = 0;
    set<str> interested_locs_;

    typedef std::mutex Lock;
    typedef std::unique_lock<Lock> UniqueLock;
    typedef std::shared_mutex RwLock;
    typedef std::unique_lock<RwLock> WriteLock;
    typedef std::shared_lock<RwLock> ReadLock;
    mutable RwLock run_entrance_lock_;
    mutable Lock run_lock_;
    mutable RwLock stat_lock_;
};

using PProgramRunnerMt = ptr<const ProgramRunnerMt>;
using PMutProgramRunnerMt = ptr<ProgramRunnerMt>;

}

#endif //IGEN4_PROGRAMRUNNERMT_H
