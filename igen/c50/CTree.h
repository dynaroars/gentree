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
    enum ExprStrat {
        FreeMix,
        DisjOfConj
    };

    explicit CTree(PMutContext ctx);

    ~CTree() override = default;

    void prepare_data(const PLocation &loc);

    void build_tree();

    vec<PConfig> &miss_configs() { return configs_[0]; }

    vec<PConfig> &hit_configs() { return configs_[1]; }

    const vec<PConfig> &miss_configs() const { return configs_[0]; }

    const vec<PConfig> &hit_configs() const { return configs_[1]; }

    z3::expr build_zexpr(ExprStrat strat) const;

    std::ostream &print_tree(std::ostream &output) const;

    std::pair<bool, int> test_config(const PConfig &conf) const;

    std::pair<bool, int> test_add_config(const PConfig &conf, bool val);

    void cleanup() override;

    std::ostream &serialize(std::ostream &out) const;

    std::istream &deserialize(std::istream &inp);

public: // For gen CEX
    int n_min_cases() const { return root_->min_cases_in_one_leaf_; };

    vec<PConfig> gather_small_leaves(int min_confs, int max_confs) const;

    void gather_nodes(vec<PCNode> &res,
                      int min_confs = 0, int max_confs = std::numeric_limits<int>::max()) const;

private:
    std::array<vec<PConfig>, 2> configs_;
    std::array<vec<sm_vec<int>>, 2> t_freq;
    vec<PConfig> t_conf;
    std::array<vec<vec<PConfig>::iterator>, 2> t_curpos;
    vec<double> t_info, t_gain;

    PMutCNode root_;

    bool default_hit_;
    bool multi_val_;
    int n_cases_;
    double avgain_wt_, mdl_wt_;

    friend class CNode;
};

using PCTree = ptr<const CTree>;
using PMutCTree = ptr<CTree>;

std::ostream &operator<<(std::ostream &output, const CTree &t);

}

#endif //IGEN4_CTREE_H
