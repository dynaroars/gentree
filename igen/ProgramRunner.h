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

BETTER_ENUM(RunnerType, int, Invalid, Simple, BuiltIn)

class ProgramRunner : public Object {
public:
    explicit ProgramRunner(PMutContext ctx);

    set<str> run(const PConfig &config) const;

private:
    RunnerType type;
    str target;
    builtin::BuiltinRunnerFn builtin_fn;

    set<str> _run_simple(const PConfig &config) const;
    set<str> _run_builtin(const PConfig &config) const;
};

using PProgramRunner = ptr<const ProgramRunner>;
using PMutProgramRunner = ptr<ProgramRunner>;

}

#endif //IGEN4_PROGRAMRUNNER_H
