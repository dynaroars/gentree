//
// Created by KH on 3/7/2020.
//

#include "CoverageStore.h"

#include <boost/container/flat_set.hpp>

namespace igen {


CoverageStore::CoverageStore(igen::PMutContext ctx) : Object(move(ctx)) {
}

void CoverageStore::register_loc(const PMutLocation &loc) {
    loc->set_id(n_locs());
    locs_.emplace_back(loc);
    clocs_.emplace_back(loc);

    PMutLocation &map_loc = map_name_loc_[loc->name()];
    CHECK_EQ(map_loc, nullptr);
    map_loc = loc;
}

void CoverageStore::register_config(const PMutConfig &config) {
    config->set_id(n_configs());
    configs_.emplace_back(config);
    cconfigs_.emplace_back(config);
    hash_configs_.add(config->hash());
}

int CoverageStore::register_cov(const PMutConfig &config, const set<str> &loc_names) {
    int n_new_locations = 0;
    if (config->id() == -1)
        register_config(config);
    vec<PMutLocation> plocs;
    plocs.reserve(loc_names.size());
    for (const str &n : loc_names) {
        PMutLocation loc = this->loc_mut(n);
        if (loc == nullptr) {
            loc = new Location(ctx(), n);
            register_loc(loc);
            n_new_locations++;
        }
        plocs.emplace_back(move(loc));
    }
    std::sort(plocs.begin(), plocs.end(), [](const auto &a, const auto &b) { return a->id() < b->id(); });
    config->cov_loc_mut_ids().reserve(plocs.size());
    for (const auto &loc : plocs) {
        link(config, loc);
    }
    return n_new_locations;
}

void CoverageStore::link(const PMutConfig &config, const PMutLocation &loc) {
    int conf_id = config->id(), loc_id = loc->id();
    CHECK(conf_id != -1 && loc_id != -1);
    DCHECK(0 <= conf_id && conf_id < n_configs() && 0 <= loc_id && loc_id < n_locs());

    config->add_cov_loc_id(loc_id);
    loc->add_cov_by_conf_id(conf_id);
}

void CoverageStore::cleanup() {
    locs_.clear();
    clocs_.clear();
    configs_.clear();
    cconfigs_.clear();
}

hash_t CoverageStore::state_hash() const {
    vec<hash_t> loc_hashes(clocs_.size());
    for (int i = 0; i < sz(clocs_); ++i) loc_hashes[i] = clocs_[i]->hash();
    vec<hash_t> ret{hash_configs_.digest(), calc_hash_128(loc_hashes)};
    return calc_hash_128(ret);
}

void intrusive_ptr_release(const CoverageStore *d) { boost::sp_adl_block::intrusive_ptr_release(d); }

void intrusive_ptr_add_ref(const CoverageStore *d) { boost::sp_adl_block::intrusive_ptr_add_ref(d); }

}