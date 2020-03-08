//
// Created by KH on 3/6/2020.
//

#ifndef IGEN4_CTREE_H
#define IGEN4_CTREE_H

#include <klib/common.h>
#include <igen/Context.h>
#include <igen/Location.h>
#include <igen/Config.h>
#include "CNode.h"

namespace igen {

class CTree : public Object {
public:
    explicit CTree(PMutContext ctx);

    void prepare_data_for_loc(const PLocation &loc);

    void build_tree();

    vec<PConfig> &miss_configs() { return configs_[0]; }

    vec<PConfig> &hit_configs() { return configs_[1]; }

    const vec<PConfig> &miss_configs() const { return configs_[0]; }

    const vec<PConfig> &hit_configs() const { return configs_[1]; }

private:
    std::array<vec<PConfig>, 2> configs_;

    std::unique_ptr<CNode> root_;

    bool default_hit_;

    friend class CNode;
};

using PCTree = ptr<const CTree>;
using PMutCTree = ptr<CTree>;

}

#endif //IGEN4_CTREE_H
