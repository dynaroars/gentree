//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_PROGRAMRUNNER_H
#define IGEN4_PROGRAMRUNNER_H

#include <igen/Context.h>
#include <igen/Config.h>

#include <boost/container/flat_set.hpp>
#include <klib/enum.h>
#include <igen/builtin/programs.h>

namespace igen {

BETTER_ENUM(RunnerType, int, Invalid, Simple, BuiltIn, GCov)

class GCovRunner;

void intrusive_ptr_release(GCovRunner *);

class ProgramRunner : public Object {
public:
    explicit ProgramRunner(PMutContext ctx);

    void init();

    set<str> run(const PConfig &config) const;

    void reset_n_runs(int val = 0) { n_runs_ = val; }

    int n_runs() const { return n_runs_; };

    void cleanup() override;

private:
    RunnerType type;
    str target;
    builtin::BuiltinRunnerFn builtin_fn;
    mutable int n_runs_ = 0;

    set<str> _run_simple(const PConfig &config) const;

    set<str> _run_builtin(const PConfig &config) const;

    ptr<GCovRunner> gcov_runner_;

    set<str> _run_gcov(const PConfig &config) const;
};

using PProgramRunner = ptr<const ProgramRunner>;
using PMutProgramRunner = ptr<ProgramRunner>;

}

#endif //IGEN4_PROGRAMRUNNER_H
