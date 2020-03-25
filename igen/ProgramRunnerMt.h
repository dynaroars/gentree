//
// Created by kh on 3/25/20.
//

#ifndef IGEN4_PROGRAMRUNNERMT_H
#define IGEN4_PROGRAMRUNNERMT_H

#include <igen/Context.h>
#include <igen/Config.h>
#include <igen/ProgramRunner.h>

#include <klib/WorkQueue.h>

#include <boost/timer/timer.hpp>

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

    int n_runs() const;

    int n_cache_hit() const { return n_cache_hit_; };

    void cleanup() override;

    void flush_cachedb();

    int n_locs() const;

    boost::timer::cpu_times timer_elapsed() const;

private:
    int n_threads_ = 1;
    int n_cache_hit_ = 0, n_unflushed_write_ = 0;
    bool has_cache{}, allow_cache_read{}, allow_cache_write{}, cache_only{};

    std::unique_ptr<rocksdb::DB> cachedb_;
    std::unique_ptr<rocksdb::ReadOptions> cachedb_readopts_{};
    std::unique_ptr<rocksdb::WriteOptions> cachedb_writeopts_{};

    vec<PMutProgramRunner> runners_;
    WorkQueue work_queue_;
};

}

#endif //IGEN4_PROGRAMRUNNERMT_H
