//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_CNODE_H
#define IGEN4_CNODE_H

#include <boost/container/small_vector.hpp>
#include <boost/range/sub_range.hpp>
#include <igen/Config.h>

namespace igen {

class CTree;

class CNode;

using PCNode = ptr<const CNode>;
using PMutCNode = ptr<CNode>;

class CNode : public intrusive_ref_base_mt<CNode> {
public:
    CNode(CTree *tree, CNode *parent, int id, std::array<boost::sub_range<vec<PConfig>>, 2> configs);

    ~CNode() = default;

    [[nodiscard]] boost::sub_range<vec<PConfig>> &miss_configs() { return configs_[0]; }

    [[nodiscard]] const boost::sub_range<vec<PConfig>> &miss_configs() const { return configs_[0]; }

    [[nodiscard]] boost::sub_range<vec<PConfig>> &hit_configs() { return configs_[1]; }

    [[nodiscard]] const boost::sub_range<vec<PConfig>> &hit_configs() const { return configs_[1]; }

    [[nodiscard]] int n_hits() const { return int(hit_configs().size()); }

    [[nodiscard]] int n_misses() const { return int(miss_configs().size()); }

    [[nodiscard]] int n_total() const { return n_hits() + n_misses(); }

    [[nodiscard]] bool is_leaf() const { return hit_configs().empty() || miss_configs().empty(); }

    // Return true => HIT, false => MISS
    [[nodiscard]] bool leaf_value() const;

    int depth() const { return depth_; }

    int n_childs() const { return int(childs.size()); }

    // SEMI PRIVATE
    int n_min_cases() const { return min_cases_in_one_leaf_; }

    vec<ptr<Config>> gen_one_convering_configs(int lim = std::numeric_limits<int>::max()) const;

    std::ostream &serialize(std::ostream &out, bool &last_is_char) const;

    std::istream &deserialize(std::istream &inp);

private:
    bool evaluate_split();

    z3::expr build_zexpr_mixed() const;

    void build_zexpr_disj_conj(z3::expr_vector &vec_res, const expr &cur_expr) const;

    void print_node(std::ostream &output, str &prefix) const;

    std::pair<bool, int> test_config(const PConfig &conf) const;

    std::pair<bool, int> test_add_config(const PConfig &conf, bool val);

private:
    CTree *tree;
    CNode *parent;
    int id_, depth_;
    std::array<boost::sub_range<vec<PConfig>>, 2> configs_;

    PVarDomain split_by;
    vec<PMutCNode> childs;
    vec<bool> tested_vars_;

    friend class CTree;

    const PDomain &dom() const;

    const PVarDomain &dom(int var_id) const;

private: // For gen CEX
    int min_cases_in_one_leaf_;
    vec<PConfig> new_configs_;

    void gather_small_leaves(vec<PConfig> &res, int min_confs, int max_confs, const PMutConfig &curtpl) const;

    void gather_leaves_nodes(vec<ptr<const CNode>> &res, int min_confs, int max_confs) const;

public:
    void gen_tpl(Config &conf) const;

private: // TEMP DATA
    int possible;
    double base_info, avgain, mdl, mingain;
    int splitvar = -1, find_pass;

    void calc_freq();

    void calc_inf_gain();

    int select_best_var(bool first_pass);

    void create_childs();

    std::ostream &print_tmp_state(std::ostream &output, const str &indent = "    ") const;
};


}

#endif //IGEN4_CNODE_H
