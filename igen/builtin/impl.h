//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_IMPL_H
#define IGEN4_IMPL_H

#include <klib/common.h>
#include <igen/Config.h>
#include <boost/container/flat_set.hpp>
#include "programs.h"

namespace igen::builtin {

struct Program {
    str name;
    str vars;
    vec<int> dom;
    std::function<set<str>(const igen::PConfig &config)> fn;
};

int register_builtin_program(Program p);
}

#define VARS(...) __VA_ARGS__
#define DOMS(...) __VA_ARGS__
#define RETURN return _set_locs
#define LOC(name) _set_locs.insert(name)
#define NUMARGS(...)  (std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)

#define EVAL_STR(...) #__VA_ARGS__

#define FN(name, vars, doms, code)                                                                          \
    namespace igen::builtin {                                                                               \
        int _registered_##name = register_builtin_program({                                                 \
            #name, EVAL_STR(vars), {doms},                                                                  \
            [](const PConfig &config) -> set<str> {                                                         \
                const size_t NVARS = NUMARGS(doms);                                                         \
                std::array<int, NVARS> _array_values;                                                       \
                const auto &_vector_values = config->values();                                              \
                CHECK_EQ(_vector_values.size(), _array_values.size());                                      \
                std::copy(_vector_values.begin(), _vector_values.begin() + NVARS, _array_values.begin());   \
                                                                                                            \
                set<str> _set_locs;                                                                         \
                auto[vars] = _array_values;                                                                 \
                { code; }                                                                                   \
                return _set_locs;                                                                           \
            }                                                                                               \
        });                                                                                                 \
    }

#define FN_MODULE(name) namespace igen::builtin { void _fnmodule_##name() {} }
#define FN_MODULE_USE(name) namespace igen::builtin { void _fnmodule_##name(); void _initfn_##name() { _fnmodule_##name(); } }

#endif //IGEN4_IMPL_H
