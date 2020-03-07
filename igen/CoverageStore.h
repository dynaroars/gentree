//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_COVERAGESTORE_H
#define IGEN4_COVERAGESTORE_H

#include "Context.h"

namespace igen {

class CoverageStore : public Object {
public:
    explicit CoverageStore(PMutContext ctx);

private:
};

using PCoverageStore = ptr<const CoverageStore>;
using PMutCoverageStore = ptr<CoverageStore>;

}

#endif //IGEN4_COVERAGESTORE_H
