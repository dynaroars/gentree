//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_PROGRAMRUNNER_H
#define IGEN4_PROGRAMRUNNER_H

#include <igen/Context.h>
#include <igen/Config.h>

#include <boost/container/flat_set.hpp>
#include <klib/enum.h>

namespace igen {

BETTER_ENUM(RunnerType, int, Invalid, Simple)

class ProgramRunner : public Object {
public:
    explicit ProgramRunner(PMutContext ctx);

    set<str> run(PConfig config) const;

private:
    RunnerType type;
};

using PProgramRunner = ptr<const ProgramRunner>;
using PMutProgramRunner = ptr<ProgramRunner>;

}

#endif //IGEN4_PROGRAMRUNNER_H
