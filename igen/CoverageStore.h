//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_COVERAGESTORE_H
#define IGEN4_COVERAGESTORE_H

#include "Context.h"
#include "Location.h"
#include "Config.h"

namespace igen {

class CoverageStore : public Object {
public:
    explicit CoverageStore(PMutContext ctx);

    const PLocation &loc(int id) const { return clocs_.at(id); }

    const PMutLocation &loc_mut(int id) const { return locs_.at(id); }

    PLocation loc(const str &name) const {
        auto it = map_name_loc_.find(name);
        return (it == map_name_loc_.end()) ? nullptr : it->second;
    }

    PMutLocation loc_mut(const str &name) const {
        auto it = map_name_loc_.find(name);
        return (it == map_name_loc_.end()) ? nullptr : it->second;
    }

    const PConfig &config(int id) const { return cconfigs_.at(id); }

    const vec<PConfig> &configs() const { return cconfigs_; }

    int n_locs() const { return int(locs_.size()); }

    int n_configs() const { return int(configs_.size()); }

    void register_loc(const PMutLocation &loc);

    void register_config(const PMutConfig &config);

    // Return no. of new Locations
    int register_cov(const PMutConfig &config, const set<str> &loc_names);

    void link(const PMutConfig &config, const PMutLocation &loc);

    void cleanup() override;

private:
    vec<PMutLocation> locs_;
    vec<PLocation> clocs_;

    vec<PMutConfig> configs_;
    vec<PConfig> cconfigs_;

    map<str, PMutLocation> map_name_loc_;
};

using PCoverageStore = ptr<const CoverageStore>;
using PMutCoverageStore = ptr<CoverageStore>;

}

#endif //IGEN4_COVERAGESTORE_H
