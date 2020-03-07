//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_DOMAIN_H
#define IGEN4_DOMAIN_H

#include "Context.h"
#include "Config.h"
#include <z3++.h>

namespace igen {

class VarEntry : public Object {
private:
    friend class Domain;

    int id_;
    str name_;
    vec<std::string> labels_;

private:
    expr zvar;
    vec<expr> zvar_eq_val;

public:
    explicit VarEntry(PMutContext ctx);

    int n_values() const { return (int) labels_.size(); }

    int id() const { return id_; }

    const str &name() const { return name_; };

    const vec<str> &labels() const { return labels_; };

    expr eq(int val) const;
};

using PVarEntry = ptr<const VarEntry>;
using PMutVarEntry = ptr<VarEntry>;

//===================================================================================

class Domain : public Object {
public:
    explicit Domain(PMutContext ctx);

    std::istream &parse(std::istream &input);

    int n_all_values() const { return n_all_values_; }

    int n_vars() const { return int(vars.size()); }

private:
    vec<PMutVarEntry> vars;
    vec<PVarEntry> cvars;
    int n_all_values_ = 0;

    friend std::ostream &operator<<(std::ostream &output, const Domain &d);

public:
    vec<PMutVarEntry>::const_iterator begin() { return vars.begin(); }

    vec<PMutVarEntry>::const_iterator end() { return vars.end(); }

    vec<PVarEntry>::const_iterator begin() const { return cbegin(); }

    vec<PVarEntry>::const_iterator end() const { return cend(); }

    vec<PVarEntry>::const_iterator cbegin() const { return cvars.begin(); }

    vec<PVarEntry>::const_iterator cend() const { return cvars.end(); }


    PVarEntry operator[](size_t idx) const { return vars.at(idx); }

    const PMutVarEntry &operator[](size_t idx) { return vars.at(idx); }
};

std::istream &operator>>(std::istream &input, Domain &d);

std::ostream &operator<<(std::ostream &output, const Domain &d);

using PDomain = ptr<const Domain>;
using PMutDomain = ptr<Domain>;

}

#endif //IGEN4_DOMAIN_H
