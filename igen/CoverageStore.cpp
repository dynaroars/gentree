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
    for (const auto &loc : plocs) {
        link(config, loc);
    }
    return n_new_locations;
}

void CoverageStore::link(const PMutConfig &config, const PMutLocation &loc) {
    int conf_id = config->id(), loc_id = loc->id();
    CHECK(conf_id != -1 && loc_id != -1);
    DCHECK(0 <= conf_id && conf_id < n_configs() && 0 <= loc_id && loc_id < n_locs());

    CHECK(config->cov_loc_mut_ids().empty() || config->cov_loc_mut_ids().back() < loc_id);
    config->cov_loc_mut_ids().push_back(loc_id);
    CHECK(loc->cov_by_mut_ids().empty() || loc->cov_by_mut_ids().back() < conf_id);
    loc->cov_by_mut_ids().push_back(conf_id);
}

void CoverageStore::cleanup() {
    locs_.clear();
    clocs_.clear();
    configs_.clear();
    cconfigs_.clear();
}

void intrusive_ptr_release(CoverageStore *d) { boost::sp_adl_block::intrusive_ptr_release(d); }


}