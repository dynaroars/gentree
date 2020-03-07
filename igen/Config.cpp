//
// Created by KH on 3/5/2020.
//

#include "Config.h"
#include "Domain.h"

namespace igen {


Config::Config(PMutContext ctx, int id)
        : Object(move(ctx)), id_(id), values_(size_t(dom()->n_vars())) {

}

vec<str> Config::value_labels() const {
    vec<str> labels(size_t(dom()->n_vars()));
    CHECK_EQ(labels.size(), values_.size());
    for (int i = 0; i < int(labels.size()); ++i) {
        labels[i] = dom()->label(i, values_[i]);
    }
    return labels;
}


std::ostream &operator<<(std::ostream &output, const Config &d) {
    output << "Config " << d.id() << ": ";
    bool first_var = true;
    for (auto &e : d) {
        if (!first_var) output << ", "; else first_var = false;
        output << e.name() << ' ' << e.label();
    }
    return output;
}

}