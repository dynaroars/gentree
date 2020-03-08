//
// Created by KH on 3/6/2020.
//

#ifndef IGEN4_CTREE_H
#define IGEN4_CTREE_H

#include <klib/common.h>
#include <igen/Context.h>
#include <igen/Location.h>
#include <igen/Config.h>

namespace igen {

class CTree : public Object {
public:
    explicit CTree(PMutContext ctx);

    void prepare_data_for_loc(const PLocation &loc);

private:
    vec<PConfig> hit_configs;
    vec<PConfig> miss_configs;
};

using PCTree = ptr<const CTree>;
using PMutCTree = ptr<CTree>;

}

#endif //IGEN4_CTREE_H
