//
// Created by KH on 3/7/2020.
//

#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "programs.h"
#include "impl.h"


FN_MODULE_USE(fn_1)
FN_MODULE_USE(fn_2)


namespace igen::builtin {

map<str, Program> &_mapprog() {
    static map<str, Program> _inst;
    return _inst;
}

void normalize_name(str &s) {
    boost::replace_all(s, "@", "");
    boost::replace_all(s, ".dom", "");
    boost::replace_all(s, ".exe", "");
}

void check_name(const str &name) {
    if (!_mapprog().count(name)) {
        throw std::runtime_error(fmt::format("Builtin runner {} not found", name));
    }
}

str get_dom_str(str name) {
    normalize_name(name);
    //CHECKF(_mapprog().count(name), "Builtin runner {} not found", name);
    check_name(name);

    const Program &prog = _mapprog()[name];
    const auto &dom = prog.dom;
    str vars = prog.vars;
    for (char &c : vars) if (c == ',') c = '\n';
    std::stringstream ss(vars), sout;

    int i = 0;
    str vname;
    while (ss >> vname) {
        sout << vname << ' ';
        for (int j = 0; j < dom[i]; ++j)
            sout << j << ' ';
        sout << '\n';
        i++;
        CHECK_LE(i, dom.size());
    }

    return sout.str();
}

std::function<set<str>(const igen::PConfig &config)> get_fn(str name) {
    normalize_name(name);
    //CHECKF(_mapprog().count(name), "Builtin runner {} not found", name);
    check_name(name);
    return _mapprog()[name].fn;
}

const str &get_src(str name) {
    normalize_name(name);
    check_name(name);
    return _mapprog()[name].src;
}

int register_builtin_program(Program p) {
    CHECK_EQ(_mapprog().count(p.name), 0);
    _mapprog()[p.name] = std::move(p);
    return 0;
}

}