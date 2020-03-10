//
// Created by KH on 3/5/2020.
//

#include <fstream>
#include <igen/builtin/programs.h>
#include "Domain.h"
#include "Config.h"

#include <boost/container/flat_set.hpp>
#include <boost/iterator/counting_iterator.hpp>

#include <klib/random.h>

namespace igen {

VarDomain::VarDomain(PMutContext ctx) : Object(move(ctx)), zvar_(zctx()) {}

//===================================================================================

str Domain::STR_VALUE_ANY = "*";

void intrusive_ptr_release(Domain *d) {
    boost::sp_adl_block::intrusive_ptr_release(d);
}

Domain::Domain(PMutContext _ctx)
        : Object(move(_ctx)), vars_expr_vector_(ctx()->zctx()), expr_all_asserts_(ctx()->zctx()) {
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
    bool assigned_expr_all_asserts = false;
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
        if (n_vals != 2) {
            expr e = 0 <= zvar && zvar < n_vals;
            zsolver().add(e);
            if (assigned_expr_all_asserts) expr_all_asserts_ = expr_all_asserts_ && e;
            else expr_all_asserts_ = e, assigned_expr_all_asserts = true;
        }

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
            expr e = (n_vals == 2 ? (i ? zvar : !zvar) : (zvar == ex_val));
            entry->zvar_eq_val.push_back(e);
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

vec<ptr<Config>> Domain::gen_all_configs() const {
    PMutConfig cfg = new Config(ctx_mut());
    cfg->set_all(-1);
    return gen_all_configs(cfg);
}

vec<ptr<Config>> Domain::gen_one_convering_configs() const {
    PMutConfig cfg = new Config(ctx_mut());
    cfg->set_all(-1);
    return gen_one_convering_configs(cfg);
}

vec<PMutConfig> Domain::gen_all_configs(const PConfig &templ) const {
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

vec<PMutConfig> Domain::gen_one_convering_configs(const PConfig &templ) const {
    vec<PMutConfig> ret;

    vec<set<int>> SetVAL;
    for (int i = 0; i < n_vars(); i++) {
        if (templ->get(i) == -1) {
            SetVAL.emplace_back(boost::counting_iterator<int>(0), boost::counting_iterator<int>(n_values(i)));
        } else {
            SetVAL.emplace_back(set<int>{templ->get(i)});
        }
    }

    int n_finished = 0;
    while (n_finished < n_vars()) {
        PMutConfig conf = new Config(ctx_mut());
        for (int i = 0; i < n_vars(); i++) {
            set<int> &st = SetVAL[i];
            if (!st.empty()) {
                auto it = Rand.get(st);
                conf->set(i, *it);
                st.erase(it);
                if (st.empty())
                    n_finished++;
            } else {
                conf->set(i, Rand.get(n_values(i)));
            }
        }
        ret.emplace_back(move(conf));
    }

    return ret;
}

}