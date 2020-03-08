//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_PROGRAMS_H
#define IGEN4_PROGRAMS_H

#include <klib/common.h>
#include <igen/Config.h>

namespace igen::builtin {

str get_dom_str(str name);

using BuiltinRunnerFn = std::function<set<str>(const igen::PConfig &config)>;

BuiltinRunnerFn get_fn(str name);

}

#endif //IGEN4_PROGRAMS_H
