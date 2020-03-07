//
// Created by KH on 3/5/2020.
//

#include "Domain.h"
#include "Config.h"

namespace igen {

VarDomain::VarDomain(PMutContext ctx) : Object(move(ctx)), zvar(zctx()) {}

expr VarDomain::eq(int val) const { return zvar_eq_val.at(val); }

//===================================================================================

str Domain::STR_VALUE_ANY = "?";

void intrusive_ptr_release(Domain *d) {
    intrusive_ptr_add_ref(d);
}

Domain::Domain(PMutContext ctx) : Object(move(ctx)) {

}

std::istream &Domain::parse(std::istream &input) {
    CHECK(input);

    n_all_values_ = 0;

    std::string line;
    while (std::getline(input, line)) {
        std::stringstream ss(line);
        str name;
        vec<str> labels;

        ss >> name;
        if (name.empty() || name[0] == '#') continue;

        std::string val;
        while (ss >> val) {
            if (val.empty() || val[0] == '#') break;
            labels.push_back(std::move(val));
        }
        int n_vals = (int) labels.size();

        //===
        z3::expr zvar = zctx().int_const(name.c_str());
        zsolver().add(0 <= zvar && zvar < n_vals);

        PMutVarDomain entry = new VarDomain(ctx_);
        vars.emplace_back(entry);
        cvars.emplace_back(entry);

        entry->id_ = (int) vars.size() - 1;
        entry->name_ = name;
        entry->labels_ = move(labels);

        entry->zvar_eq_val.reserve(n_vals);
        for (int i = 0; i < n_vals; i++)
            entry->zvar_eq_val.push_back(zvar == i);

        n_all_values_ += n_vals;
    }

    CHECK_EQ(vars.size(), cvars.size());
    return input;
}


std::istream &operator>>(std::istream &input, Domain &d) {
    return d.parse(input);
}

std::ostream &operator<<(std::ostream &output, const Domain &d) {
    fmt::print(output, "Domain[{}]: ", d.n_vars());
    bool first_var = true;
    for (const PVarDomain &e : d) {
        if (!first_var) output << "; "; else first_var = false;

        fmt::print(output, "({}) {} = ", e->n_values(), e->name());
        bool first_value = true;
        for (const str &lbl : e->labels()) {
            if (!first_value) output << " "; else first_value = false;
            output << lbl;
        }
    }
    return output;
}

vec<ptr<Config>> Domain::gen_all_configs(vec<int> fixed) const {
    CHECK(fixed.empty() || int(fixed.size()) == dom()->n_vars());
    vec<PMutConfig> configs;

    return configs;
}

}