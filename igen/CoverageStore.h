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

    const PConfig &config(int id) const { return cconfigs_.at(id); }

private:
    vec<PMutLocation> locs_;
    vec<PLocation> clocs_;

    vec<PMutConfig> configs_;
    vec<PConfig> cconfigs_;
};

using PCoverageStore = ptr<const CoverageStore>;
using PMutCoverageStore = ptr<CoverageStore>;

}

#endif //IGEN4_COVERAGESTORE_H
