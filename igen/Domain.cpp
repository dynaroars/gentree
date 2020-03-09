//
// Created by KH on 3/5/2020.
//

#include <fstream>
#include <igen/builtin/programs.h>
#include "Domain.h"
#include "Config.h"

namespace igen {

VarDomain::VarDomain(PMutContext ctx) : Object(move(ctx)), zvar_(zctx()) {}

//===================================================================================

str Domain::STR_VALUE_ANY = "?";

void intrusive_ptr_release(Domain *d) {
    boost::sp_adl_block::intrusive_ptr_release(d);
}

Domain::Domain(PMutContext _ctx) : Object(move(_ctx)), vars_expr_vector_(ctx()->zctx()) {
    str filepath;
    if (ctx()->has_option("dom")) {
        filepath = ctx()->get_option_as<str>("dom");
    } else {
        filepath = ctx()->get_option_as<str>("filestem") + ".dom";
    }
    if (filepath.at(0) == '@') {
        std::stringstream ss(builtin::get_dom_str(filepath));
        ss >> (*this);
    } else {
        std::ifstream ifs_dom(filepath);
        CHECKF(ifs_dom, "Bad dom input file: {}", filepath);
        ifs_dom >> (*this);
    }
    LOG(INFO, "Loaded dom {}: ", filepath) << (*this);
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
        CHECK_GT(n_vals, 0);

        //===
        z3::expr zvar = (n_vals == 2) ? zctx().bool_const(name.c_str()) : zctx().int_const(name.c_str());
        if (n_vals != 2)
            zsolver().add(0 <= zvar && zvar < n_vals);

        PMutVarDomain entry = new VarDomain(ctx());
        vars_.emplace_back(entry);
        cvars_.emplace_back(entry);

        entry->id_ = (int) vars_.size() - 1;
        entry->name_ = name;
        entry->labels_ = move(labels);
        entry->zvar_ = zvar;

        entry->zvar_eq_val.reserve(n_vals);
        for (int i = 0; i < n_vals; i++) {
            expr ex_val = (n_vals == 2) ? ctx()->zbool(i) : ctx()->zctx().int_val(i);
            entry->zvals_.push_back(ex_val);
            entry->zvar_eq_val.push_back(zvar == ex_val);
        }

        n_all_values_ += n_vals;
    }

    vars_expr_vector_.resize(unsigned(vars_.size()));
    for (int i = 0; i < n_vars(); ++i) {
        auto ref = vars_[i]->zvar();
        vars_expr_vector_.set(i, ref);
    }

    CHECK_EQ(vars_.size(), cvars_.size());
    return input;
}

void Domain::cleanup() {
    vars_.clear();
    cvars_.clear();
}


std::istream &operator>>(std::istream &input, Domain &d) {
    return d.parse(input);
}

std::ostream &operator<<(std::ostream &output, const Domain &d) {
    fmt::print(output, "Domain[{}]: ", d.n_vars());
    bool first_var = true;
    for (const auto &e : d) {
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

vec<PMutConfig> Domain::gen_all_configs(PConfig templ) const {
    vec<PMutConfig> configs;
    vec<int> values(size_t(dom()->n_vars()));
    auto recur = [this, &templ, &configs, &values](int var_id, const auto &recur) -> void {
        if (var_id == dom()->n_vars()) {
            configs.emplace_back(new Config(ctx_mut(), values));
            return;
        }
        if (templ->get(var_id) != -1) {
            values[var_id] = templ->get(var_id);
            recur(var_id + 1, recur);
            return;
        }
        for (int i = 0; i < dom()->n_values(var_id); ++i) {
            values[var_id] = i;
            recur(var_id + 1, recur);
        }
    };
    recur(0, recur);
    return configs;
}

vec<ptr<Config>> Domain::gen_all_configs() const {
    PMutConfig cfg = new Config(ctx_mut());
    cfg->set_all(-1);
    return gen_all_configs(cfg);
}

}