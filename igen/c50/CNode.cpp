//
// Created by KH on 3/7/2020.
//

#include "CNode.h"
#include "CTree.h"

namespace igen {


CNode::CNode(CTree &tree, CNode &parent, std::array<boost::sub_range<vec<PConfig>>, 2> configs) :
        tree(tree), parent(parent), configs_(std::move(configs)) {
}

PDomain CNode::dom() const {
    return tree.ctx()->dom();
}

bool CNode::evaluate_split() {
    if (is_leaf())
        return false;

    vec<vec<int>> freq[2] = {
            dom()->create_vec_vars_values<int>(),
            dom()->create_vec_vars_values<int>(),
    };
    for (int hit = 0; hit <= 1; ++hit) {
        for (const auto &c : configs_[hit]) {
            const auto &vals = c->values();
            for (int var_id = 0; var_id < dom()->n_vars(); ++var_id) {
                freq[hit][var_id][vals[var_id]]++;
            }
        }
    }

    return split_by != nullptr;
}


}