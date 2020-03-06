//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_DOMAIN_H
#define IGEN4_DOMAIN_H

#include "Context.h"
#include "Config.h"
#include <z3++.h>

namespace igen {

struct VarEntry : Object {
private:
    friend class Domain;

    int id_;
    str name_;
    vec<std::string> labels_;

private:
    expr zvar;
    vec<expr> zvar_eq_val;

public:
    explicit VarEntry(PContext ctx);

    int n_vars() const { return (int) labels_.size(); }

    int id() const { return id_; }

    const str &name() const { return name_; };

    const vec<str> &labels() const { return labels_; };

    expr eq(int val) const;
};

using PVarEntry = ptr<VarEntry>;

//===================================================================================

class Domain : public Object {
public:
    explicit Domain(PContext ctx);

    std::istream &parse(std::istream &input);

    int n_all_values() const { return n_all_values_; }

private:
    vec<PVarEntry> vars;
    int n_all_values_;
};

std::istream &operator>>(std::istream &input, Domain &d);

using PDomain = ptr<Domain>;

}

#endif //IGEN4_DOMAIN_H
