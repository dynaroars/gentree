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
    vec <std::string> labels_;

private:
    expr zvar_;
    vec <expr> zvar_eq_val;
    vec <expr> zvals_;

public:
    explicit VarDomain(PMutContext ctx);

    int n_values() const { return (int) labels_.size(); }

    int id() const { return id_; }

    const str &name() const { return name_; };

    const vec <str> &labels() const { return labels_; }

    const str &label(int v) const { return labels_.at(size_t(v)); }

    const expr &eq(int val) const { return zvar_eq_val.at(size_t(val)); };

    const expr &val(int v) const { return zvals_.at(size_t(v)); }

    const expr &zvar() const { return zvar_; }

    const vec <expr> &zvals() const { return zvals_; }

    const expr &zval(int v) const { return zvals_[size_t(v)]; }
};

using PVarDomain = ptr<const VarDomain>;
using PMutVarDomain = ptr<VarDomain>;

//===================================================================================

class Domain : public Object {
public:
    explicit Domain(PMutContext ctx);

    std::istream &parse(std::istream &input);

    int n_all_values() const { return n_all_values_; }

    int n_vars() const { return int(vars_.size()); }

    int n_values(int var_id) const { return cvars_.at(size_t(var_id))->n_values(); }

    const str &name(int var_id) const { return cvars_.at(size_t(var_id))->name(); }

    const vec <str> &labels(int var_id) const { return cvars_.at(size_t(var_id))->labels(); }

    const str &label(int var_id, int value) const {
        return value == -1 ? STR_VALUE_ANY : labels(var_id).at(size_t(value));
    }

    const vec <PVarDomain> &vars() const { return cvars_; }

    const PVarDomain &var(int i) const { return cvars_[size_t(i)]; }

    const z3::expr_vector &vars_expr_vector() const { return vars_expr_vector_; }

    const expr &expr_all_asserts() const { return expr_all_asserts_; }

    const z3::func_decl_vector &func_decl_vector() const { return func_decl_vector_; }

    void cleanup() override;

public:
    vec <ptr<Config>> gen_all_configs() const;

    vec <ptr<Config>> gen_all_configs(const ptr<const Config> &templ) const;

    vec <ptr<Config>> gen_one_convering_configs() const;

    vec <ptr<Config>> gen_one_convering_configs(const ptr<const Config> &templ,
                                                int lim = std::numeric_limits<int>::max()) const;

    template<typename T>
    vec <T> create_vec_vars() const { return vec<T>(n_vars()); }

    template<typename T>
    vec <vec<T>> create_vec_vars_values() const {
        auto res = vec<vec<T>>(n_vars());
        for (int i = 0; i < n_vars(); ++i)
            res[i].resize(n_values(i));
        return res;
    }

private:
    vec <PMutVarDomain> vars_;
    vec <PVarDomain> cvars_;
    int n_all_values_ = 0;
    z3::expr_vector vars_expr_vector_;
    z3::func_decl_vector func_decl_vector_;
    expr expr_all_asserts_;

    friend std::ostream &operator<<(std::ostream &output, const Domain &d);

    static str STR_VALUE_ANY;

public:
    vec<PMutVarDomain>::const_iterator begin() { return vars_.begin(); }

    vec<PMutVarDomain>::const_iterator end() { return vars_.end(); }

    vec<PVarDomain>::const_iterator begin() const { return cbegin(); }

    vec<PVarDomain>::const_iterator end() const { return cend(); }

    vec<PVarDomain>::const_iterator cbegin() const { return cvars_.begin(); }

    vec<PVarDomain>::const_iterator cend() const { return cvars_.end(); }


    PVarDomain operator[](size_t idx) const { return vars_.at(idx); }

    const PMutVarDomain &operator[](size_t idx) { return vars_.at(idx); }
};

std::istream &operator>>(std::istream &input, Domain &d);

std::ostream &operator<<(std::ostream &output, const Domain &d);

using PDomain = ptr<const Domain>;
using PMutDomain = ptr<Domain>;

}

#endif //IGEN4_DOMAIN_H
