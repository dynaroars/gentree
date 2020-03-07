//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_DOMAIN_H
#define IGEN4_DOMAIN_H

#include "Context.h"
#include <z3++.h>

namespace igen {

class Config;

class VarDomain : public Object {
private:
    friend class Domain;

    int id_;
    str name_;
    vec<std::string> labels_;

private:
    expr zvar;
    vec<expr> zvar_eq_val;

public:
    explicit VarDomain(PMutContext ctx);

    int n_values() const { return (int) labels_.size(); }

    int id() const { return id_; }

    const str &name() const { return name_; };

    const vec<str> &labels() const { return labels_; };

    expr eq(int val) const;
};

using PVarDomain = ptr<const VarDomain>;
using PMutVarDomain = ptr<VarDomain>;

//===================================================================================

class Domain : public Object {
public:
    explicit Domain(PMutContext ctx);

    std::istream &parse(std::istream &input);

    int n_all_values() const { return n_all_values_; }

    int n_vars() const { return int(vars.size()); }

    int n_values(int var_id) const { return cvars.at(size_t(var_id))->n_values(); }

    const str &name(int var_id) const { return cvars.at(size_t(var_id))->name(); }

    const vec<str> &labels(int var_id) const { return cvars.at(size_t(var_id))->labels(); }

    const str &label(int var_id, int value) const {
        return value == -1 ? STR_VALUE_ANY : labels(var_id).at(size_t(value));
    }

public:
    vec<ptr<Config>> gen_all_configs() const;
    vec<ptr<Config>> gen_all_configs(ptr<const Config> templ) const;

private:
    vec<PMutVarDomain> vars;
    vec<PVarDomain> cvars;
    int n_all_values_ = 0;

    friend std::ostream &operator<<(std::ostream &output, const Domain &d);

    static str STR_VALUE_ANY;

public:
    vec<PMutVarDomain>::const_iterator begin() { return vars.begin(); }

    vec<PMutVarDomain>::const_iterator end() { return vars.end(); }

    vec<PVarDomain>::const_iterator begin() const { return cbegin(); }

    vec<PVarDomain>::const_iterator end() const { return cend(); }

    vec<PVarDomain>::const_iterator cbegin() const { return cvars.begin(); }

    vec<PVarDomain>::const_iterator cend() const { return cvars.end(); }


    PVarDomain operator[](size_t idx) const { return vars.at(idx); }

    const PMutVarDomain &operator[](size_t idx) { return vars.at(idx); }
};

std::istream &operator>>(std::istream &input, Domain &d);

std::ostream &operator<<(std::ostream &output, const Domain &d);

using PDomain = ptr<const Domain>;
using PMutDomain = ptr<Domain>;

}

#endif //IGEN4_DOMAIN_H
