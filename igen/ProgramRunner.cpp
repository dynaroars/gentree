//
// Created by KH on 3/7/2020.
//

#include "ProgramRunner.h"

namespace igen {


ProgramRunner::ProgramRunner(PMutContext _ctx) : Object(move(_ctx)), type(RunnerType::Invalid) {
    if (ctx()->has_option("simple-runner")) {
        type = RunnerType::Simple;
    } else {
        str str_type = ctx()->get_option_as<str>("runner");
        type = RunnerType::_from_string_nocase(str_type.c_str());
    }
}

set<str> ProgramRunner::run(const PConfig &config) const {
    set<str> locations;

    return locations;
}

}