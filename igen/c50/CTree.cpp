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
    n_cases_ = (int) cov()->configs().size();

    // PREPARE VALS
    multi_val_ = true;
    for (const auto &c : *dom()) {
        multi_val_ = (c->n_values() >= 0.3 * (n_cases_ + 1));
    }

    // MIN THRES
    if ((n_cases_ + 1) / 2 <= 500) {
        avgain_wt_ = 1.0;
        mdl_wt_ = 0.0;
    } else if ((n_cases_ + 1) / 2 >= 1000) {
        avgain_wt_ = 0.0;
        mdl_wt_ = 0.9;
    } else {
        double frac = ((n_cases_ + 1) / 2 - 500) / 500.0;
        avgain_wt_ = 1 - frac;
        mdl_wt_ = 0.9 * frac;
    }
}

void CTree::build_tree() {
    root_ = new CNode(this, nullptr, {miss_configs(), hit_configs()});
    root_->evaluate_split();
}

z3::expr CTree::build_zexpr(ExprStrat strat) const {
    CHECK_NE(root_, nullptr);
    switch (strat) {
        case FreeMix:
            return root_->build_zexpr_mixed();
        case DisjOfConj: {
            z3::expr_vector res(ctx_mut()->zctx());
            root_->build_zexpr_disj_conj(res, ctx()->ztrue());
            return z3::mk_or(res);
        }
        default:
            CHECK(0);
    }
    abort();
}

std::ostream &CTree::print_tree(std::ostream &output) const {
    CHECK(root_ != nullptr);
    str prefix;
    root_->print_node(output, prefix);
    return output;
}

std::ostream &operator<<(std::ostream &output, const CTree &t) {
    return t.print_tree(output);
}

}