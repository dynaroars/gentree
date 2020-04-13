//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_PROGRAMRUNNER_H
#define IGEN4_PROGRAMRUNNER_H

#include <igen/Context.h>
#include <igen/Config.h>
#include <igen/builtin/programs.h>

#include <boost/timer/timer.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

#include <klib/enum.h>

namespace igen {

BETTER_ENUM(RunnerType, int, Invalid, Simple, BuiltIn, GCov, Otter)

class GCovRunner;

void intrusive_ptr_release(GCovRunner *);

class ProgramRunner : public Object {
public:
    explicit ProgramRunner(PMutContext ctx, map<str, str> default_vars = {});

    void init();

    set<str> run(const PConfig &config);

    void reset_stat() { n_runs_ = 0, timer_.start(), timer_.stop(); }

    int n_runs() const { return n_runs_; };

    void cleanup() override;

    int n_locs() const;

    const boost::timer::cpu_timer &timer() const { return timer_; }

private:
    RunnerType type;
    str target;
    builtin::BuiltinRunnerFn builtin_fn;
    std::atomic<int> n_runs_ = 0;
    boost::timer::cpu_timer timer_;
    map<str, str> default_vars_;

    set<str> _run_simple(const PConfig &config) const;

    set<str> _run_builtin(const PConfig &config) const;

    ptr<GCovRunner> gcov_runner_;

    set<str> _run_gcov(const PConfig &config) const;
};

using PProgramRunner = ptr<const ProgramRunner>;
using PMutProgramRunner = ptr<ProgramRunner>;

}

#endif //IGEN4_PROGRAMRUNNER_H
