//
// Created by KH on 3/7/2020.
//

#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "programs.h"
#include "impl.h"

namespace igen::builtin {

map<str, Program> &_mapprog() {
    static map<str, Program> _inst;
    return _inst;
}

void ensure_init() {
    if (_mapprog().empty())
        init();
}

void normalize_name(str &s) {
    boost::replace_all(s, "@", "");
    boost::replace_all(s, ".dom", "");
    boost::replace_all(s, ".exe", "");
}

str get_dom_str(str name) {
    ensure_init();
    normalize_name(name);
    CHECKF(_mapprog().count(name), "Builtin runner {} not found", name);
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
    ensure_init();
    normalize_name(name);
    CHECKF(_mapprog().count(name), "Builtin runner {} not found", name);
    return _mapprog()[name].fn;
}

int register_builtin_program(Program p) {
    CHECK_EQ(_mapprog().count(p.name), 0);
    _mapprog()[p.name] = std::move(p);
    return 0;
}

}