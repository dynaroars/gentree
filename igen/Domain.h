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

    int var_idx_{};
    str name_;
    vec<std::string> labels_;

public:
    explicit VarEntry(PContext ctx);

    int n_range() const { return (int) labels_.size(); }

    int id() const { return var_idx_; }

    const str &name() const { return name_; };

    const vec<str> &labels() const { return labels_; };
};

using PVarEntry = ptr<VarEntry>;

//===================================================================================

class Domain : public Object {
public:
    explicit Domain(PContext ctx);

private:
    vec<PVarEntry> vars;
};

using PDomain = ptr<Domain>;

}

#endif //IGEN4_DOMAIN_H
