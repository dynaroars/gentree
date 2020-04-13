//
// Created by kh on 4/12/20.
//

#ifndef IGEN4_OTTERRUNNER_H
#define IGEN4_OTTERRUNNER_H

#include <klib/common.h>
#include <igen/Config.h>

#include <boost/dynamic_bitset.hpp>
#include <utility>

namespace igen {

class OtterRunner : public intrusive_ref_base_mt<OtterRunner> {
public:
    static ptr<const OtterRunner> create(const str &otter_file);

    ~OtterRunner();

    int n_locs() const { return sz(loc_names_); }

    int n_vars() const { return n_vars_; }

    set<str> run(const PConfig &config) const;

private:
    explicit OtterRunner(const str &otter_file);

    void _parse(const str &otter_file);

    bool _match(const vec<short> &a, const PConfig &b) const;

private:
    using Bitset = boost::dynamic_bitset<>;

    struct Entry {
        vec<vec<short>> confs;
        Bitset covs;

        Entry(vec<vec<short>> samples, Bitset covs) : confs(std::move(samples)), covs(std::move(covs)) {}
    };

    vec<Entry> entries_;
    vec<str> loc_names_;
    int n_vars_ = 0;
};

using POtterRunner = ptr<const OtterRunner>;

}

#endif //IGEN4_OTTERRUNNER_H
