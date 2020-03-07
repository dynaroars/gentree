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
}

void CoverageStore::register_config(const PMutConfig &config) {
    config->set_id(n_configs());
    configs_.emplace_back(config);
    cconfigs_.emplace_back(config);
}

int CoverageStore::register_cov(const PMutConfig &config, const set<str> &loc_names) {
    int n_new_locations = 0;
    if (config->id() == -1) register_config(config);
    for (const str &n : loc_names) {
        PMutLocation loc = this->loc_mut(n);
        if (loc == nullptr) {
            loc = new Location(ctx(), n);
            register_loc(loc);
            n_new_locations++;
        }
        link(config, loc);
    }
    return n_new_locations;
}

void CoverageStore::link(const PMutConfig &config, const PMutLocation &loc) {
    int conf_id = config->id(), loc_id = loc->id();
    CHECK(conf_id != -1 && loc_id != -1);
    DCHECK(0 <= conf_id && conf_id < n_configs() && 0 <= loc_id && loc_id < n_locs());
    config->cov_loc_mut_ids().push_back(loc_id);
    loc->cov_by_mut_ids().push_back(conf_id);
}

void intrusive_ptr_release(CoverageStore *d) { intrusive_ptr_add_ref(d); }


}