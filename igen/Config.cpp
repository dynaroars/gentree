//
// Created by KH on 3/5/2020.
//

#include "Config.h"
#include "Domain.h"

namespace igen {


Config::Config(PMutContext ctx, int id)
        : Object(move(ctx)), id_(id), values_(size_t(dom()->n_vars())) {
}

Config::Config(PMutContext ctx, const vec<int> &values, int id) : Config(move(ctx), id) {
    set_all(values);
}

vec<str> Config::value_labels() const {
    vec<str> labels(size_t(dom()->n_vars()));
    CHECK_EQ(labels.size(), values_.size());
    for (int i = 0; i < int(labels.size()); ++i) {
        labels[i] = dom()->label(i, values_[i]);
    }
    return labels;
}

void Config::set(int var_id, int value) {
    CHECK(0 <= var_id && var_id < dom()->n_vars() && -1 <= value && value < dom()->n_values(var_id));
    values_[var_id] = value;
}

void Config::set_all(int value) {
    for (int i = 0; i < dom()->n_vars(); ++i) {
        set(i, value);
    }
}

void Config::set_all(const vec<int> &values) {
    CHECK_EQ(values.size(), dom()->n_vars());
    for (int i = 0; i < dom()->n_vars(); ++i) {
        set(i, values[i]);
    }
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