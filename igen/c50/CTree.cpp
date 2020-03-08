//
// Created by KH on 3/6/2020.
//

#include "CTree.h"

#include <igen/CoverageStore.h>

namespace igen {


CTree::CTree(PMutContext ctx) : Object(move(ctx)) {

}

void CTree::prepare_data_for_loc(const PLocation &loc) {
    CHECK_NE(loc->id(), -1);
    auto it = loc->cov_by_ids().begin(), it_end = loc->cov_by_ids().end();
    for (const auto &c : cov()->configs()) {
        if (it != it_end && *it == c->id()) {
            ++it;
            hit_configs().emplace_back(c);
        } else {
            miss_configs().emplace_back(c);
        }
    }
    default_hit_ = hit_configs() > miss_configs();
}

void CTree::build_tree() {
    root_ = new CNode(this, nullptr, {miss_configs(), hit_configs()});
    root_->evaluate_split();
}

}