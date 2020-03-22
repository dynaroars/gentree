//
// Created by KH on 3/7/2020.
//

#include "CNode.h"
#include "CTree.h"

#include <klib/math.h>
#include <klib/random.h>
#include <klib/vecutils.h>

#include <memory>
#include <boost/container/small_vector.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/iterator/counting_iterator.hpp>

namespace igen {


static const double Epsilon = 1E-4;

static const int V_LOG_BUILD = 100;

CNode::CNode(CTree *tree, CNode *parent, int id, std::array<boost::sub_range<vec<PConfig>>, 2> configs) :
        tree(tree), parent(parent), id_(id), configs_(std::move(configs)) {
    depth_ = (parent ? parent->depth() + 1 : 0);
    if (parent) {
        tested_vars_ = parent->tested_vars_;
    } else {
        tested_vars_ = dom()->create_vec_vars<bool>();
    }
}

const PDomain &CNode::dom() const {
    return tree->ctx()->dom();
}

const PVarDomain &CNode::dom(int var_id) const {
    return dom()->vars()[var_id];
}

void CNode::calc_freq() {
    int n_vars = (int) dom()->n_vars();
    auto &freq = tree->t_freq;
    for (auto &a : freq) for (auto &b : a) for (auto &c : b) c = 0;
    for (const auto &c : configs_[0]) {
        const auto &vals = c->values();
        for (int var_id = 0; var_id < n_vars; ++var_id) {
            freq[0][var_id][vals[var_id]]++;
        }
    }
    for (const auto &c : configs_[1]) {
        const auto &vals = c->values();
        for (int var_id = 0; var_id < n_vars; ++var_id) {
            freq[1][var_id][vals[var_id]]++;
        }
    }
}

void CNode::calc_inf_gain() {
    splitvar = -1;
    double log2ntotal = log2(n_total());
    auto &freq = tree->t_freq;
    auto &info = tree->t_info, &gain = tree->t_gain;

    // Calc info
    for (int var_id = 0; var_id < dom()->n_vars(); ++var_id) {
        if (tested_vars_[var_id]) continue;
        double sum = 0;
        for (int val = 0; val < dom()->n_values(var_id); ++val) {
            int n = freq[0][var_id][val] + freq[1][var_id][val];
            sum += n * log2(n);
        }
        info[var_id] = log2ntotal - sum / n_total();
    }

    // Calc base_info
    double base_info = 0;
    for (int hit = 0; hit <= 1; ++hit) {
        int n = int(configs_[hit].size());
        base_info += n * log2(n);
    }
    base_info = log2ntotal - base_info / n_total();

    // Calc gain
    double total_gain = 0;
    for (int var_id = 0; var_id < dom()->n_vars(); ++var_id) {
        if (tested_vars_[var_id]) continue;
        double &g = (gain[var_id] = 0);
        for (int val = 0; val < dom()->n_values(var_id); ++val) {
            double sum = 0;
            int total = 0;
            for (int hit = 0; hit <= 1; ++hit) {
                int n = freq[hit][var_id][val];
                sum += n * log2(n);
                total += n;
            }
            sum = total * log2(total) - sum;
            g += sum;
        }
        g = base_info - g / n_total();
        total_gain += g;
    }

    // Calc avgain & mdl
    avgain = 0;
    possible = 0;
    for (int var = 0; var < dom()->n_vars(); ++var) {
        if (tested_vars_[var]) continue;
        if (gain[var] >= Epsilon &&
            (tree->multi_val_ || dom(var)->n_values() < 0.3 * (tree->n_cases_ + 1))) {
            possible++;
            avgain += gain[var];
        } else {
            //Gain[Att] = None;
        }
    }

    avgain /= possible;
    mdl = log2(possible) / int(hit_configs().size() + miss_configs().size());
    mingain = avgain * tree->avgain_wt_ + mdl * tree->mdl_wt_;
}

int CNode::select_best_var(bool first_pass) {
    auto &info = tree->t_info, &gain = tree->t_gain;
    splitvar = -1;
    find_pass = first_pass ? 1 : 2;
    double bestratio = -1000;
    int bestnbr = dom()->n_all_values();

    for (int var = 0; var < dom()->n_vars(); ++var) {
        if (tested_vars_[var]) continue;

        double inf = info[var];

        if (first_pass) {
            if (gain[var] < 0.999 * mingain || inf <= 0) continue;
        } else {
            if (inf <= 0) inf = Epsilon;
        }

        double val = gain[var] / inf;
        int nbr = dom()->n_values(var);

        if (val > bestratio
            || (val > 0.999 * bestratio && (nbr < bestnbr || (nbr == bestnbr && gain[var] > gain[var])))) {
            splitvar = var;
            bestratio = val;
            bestnbr = nbr;
        }
    }

    return splitvar;
}

void CNode::create_childs() {
    CHECK(0 <= splitvar && splitvar < dom()->n_vars());
    int nvalues = dom()->n_values(splitvar);
    for (int hit = 0; hit <= 1; ++hit) {
        auto &c = configs_[hit];
        std::sort(c.begin(), c.end(), [splitvar = splitvar](const auto &a, const auto &b) {
            return a->get(splitvar) < b->get(splitvar);
        });
    }

    tested_vars_[splitvar] = true;
    childs.resize(nvalues);

    vec<PConfig>::iterator beg[2], end[2] = {miss_configs().begin(), hit_configs().begin()};
    vec<PConfig>::iterator real_end[2] = {miss_configs().end(), hit_configs().end()};
    for (int val = 0; val < nvalues; ++val) {
        for (int hit = 0; hit <= 1; ++hit) {
            beg[hit] = end[hit];
            while (end[hit] != real_end[hit] && (*end[hit])->value(splitvar) == val)
                end[hit]++;
        }

        boost::sub_range<vec<PConfig>> miss_conf = {beg[0], end[0]};
        boost::sub_range<vec<PConfig>> hit_conf = {beg[1], end[1]};
        childs[val] = new CNode(tree, this, val, {miss_conf, hit_conf});
    }
}

bool CNode::evaluate_split() {
    if (is_leaf()) {
        CHECK(n_hits() == 0 || n_misses() == 0); // Same config lead to 2 different result?
        GVLOG(V_LOG_BUILD) << "\n<" << depth() << ">: " << n_total() << " cases\n    "
                           << (leaf_value() ? "HIT" : "MISS");
        min_cases_in_one_leaf_ = n_total();
        return false;
    }

    calc_freq();
    calc_inf_gain();
    if (select_best_var(true) == -1) select_best_var(false);
    CHECK_NE(splitvar, -1);
    VLOG_BLOCK(V_LOG_BUILD, print_tmp_state(log << "\n<" << depth() << ">: " << n_total() << " cases\n"));

    split_by = dom()->vars().at(splitvar);
    create_childs();

    min_cases_in_one_leaf_ = std::numeric_limits<int>::max();
    for (auto &c : childs) {
        c->evaluate_split();
        min_cases_in_one_leaf_ = std::min(min_cases_in_one_leaf_, c->min_cases_in_one_leaf_);
    }

    return split_by != nullptr;
}

std::ostream &CNode::print_tmp_state(std::ostream &output, const str &indent) const {
    auto &freq = tree->t_freq;
    auto &info = tree->t_info, &gain = tree->t_gain;
    for (const auto &var : dom()->vars()) {
        // if (info[var->id()] < 0 || gain[var->id()] < 0)
        //     continue;
        if (tested_vars_[var->id()]) continue;
        output << indent << "Var " << var->name() << ": \n";
        fmt::print(output, "{}{}[{:>4}{:>8}{:>8}]\n", indent, indent, "Val", "MISS", "HIT");
        for (int i = 0; i < var->n_values(); ++i) {
            fmt::print(output, "{}{}[{:>4}{:>8}{:>8}]\n", indent, indent,
                       i, freq[0][var->id()][i], freq[1][var->id()][i]);
        }
        fmt::print(output, "{}{}info {:.3f}, gain {:.3f}, val {:.3f}\n", indent, indent,
                   info[var->id()], gain[var->id()], gain[var->id()] / info[var->id()]);
    }
    fmt::print(output, "{}av gain={:.3f}, MDL ({}) = {:.3f}, min={:.3f}\n",
               indent, avgain, possible, mdl, mingain);
    fmt::print(output, "{}best var ", indent);
    if (splitvar == -1) {
        output << "N/A\n";
    } else {
        fmt::print(output, "{}: info {:.3f}, gain {:.3f}, val {:.3f}{}",
                   dom()->name(splitvar), info[splitvar], gain[splitvar], gain[splitvar] / info[splitvar],
                   find_pass == 1 ? "\n" : ", second-pass\n");
    }
    return output;
}

bool CNode::leaf_value() const {
    CHECK(is_leaf());
    if (hit_configs().empty() && miss_configs().empty())
        return tree->default_hit_;
    return !hit_configs().empty();
}


z3::expr CNode::build_zexpr_mixed() const {
    if (is_leaf()) {
        return tree->ctx()->zbool(leaf_value());
    }
    CHECK(splitvar != -1 && int(childs.size()) == dom(splitvar)->n_values());

    z3::expr res(tree->ctx_mut()->zctx()), e = res;
    bool empty_res = true;

    for (int val = 0; val < int(childs.size()); ++val) {
        bool need_or = false;
        if (childs[val]->is_leaf()) {
            if (childs[val]->leaf_value()) {
                e = dom(splitvar)->eq(val);
                need_or = true;
            }
        } else {
            e = dom(splitvar)->eq(val) && childs[val]->build_zexpr_mixed();
            need_or = true;
        }
        if (need_or) {
            if (empty_res) res = e, empty_res = false;
            else res = res || e;
        }
    }
    CHECK(!empty_res);
    return res;
}

void CNode::build_zexpr_disj_conj(z3::expr_vector &vec_res, const expr &cur_expr) const {
    if (is_leaf() && leaf_value()) {
        vec_res.push_back(cur_expr);
        return;
    }
    for (int val = 0; val < int(childs.size()); ++val) {
        expr next_expr = depth_ == 0 ?
                         dom(splitvar)->eq(val) :
                         cur_expr && dom(splitvar)->eq(val);
        childs[val]->build_zexpr_disj_conj(vec_res, next_expr);
    }
}

void CNode::print_node(std::ostream &output, str &prefix) const {
    if (is_leaf()) {
        output << (leaf_value() ? "HIT" : "MISS") << " (" << n_total() << ")\n";
        return;
    }
    CHECK_NE(splitvar, -1);
    CHECK_GE(n_childs(), 2);
    const str &var_name = dom(splitvar)->name();

    prefix.append(":   ");
    for (int v = 0; v < n_childs(); ++v) {
        if (v == n_childs() - 1) prefix.at(prefix.size() - 4) = ' ';

        const PMutCNode n = childs[v];
        if (v == 0) {
            if (prefix.size() >= 8)
                output << std::string_view(prefix.data(), prefix.size() - 8) << ":...";
        } else {
            output << std::string_view(prefix.data(), prefix.size() - 4);
        }
        output << var_name << " = " << dom(splitvar)->label(v) << (n->is_leaf() ? ": " : ":\n");
        n->print_node(output, prefix);
    }
    prefix.resize(prefix.size() - 4);
}

std::pair<bool, int> CNode::test_config(const PConfig &conf) const {
    if (is_leaf()) return {leaf_value(), n_total()};
    CHECK_NE(splitvar, -1);
    return childs.at(size_t(conf->get(splitvar)))->test_config(conf);
}

std::pair<bool, int> CNode::test_add_config(const PConfig &conf, bool val) {
    if (is_leaf()) {
        bool leaf_val = leaf_value();
        if (leaf_val == val) {
            min_cases_in_one_leaf_++;
            new_configs_.push_back(conf);
            CHECK_EQ(min_cases_in_one_leaf_, n_total() + new_configs_.size());
        }
        return {leaf_val, n_total()};
    }
    CHECK_NE(splitvar, -1);
    auto res = childs.at(size_t(conf->get(splitvar)))->test_add_config(conf, val);
    min_cases_in_one_leaf_ = std::numeric_limits<int>::max();
    for (auto &c : childs) {
        min_cases_in_one_leaf_ = std::min(min_cases_in_one_leaf_, c->min_cases_in_one_leaf_);
    }
    return res;
}

// =====================================================================================================================

void CNode::gather_small_leaves(vec<PConfig> &res, int min_confs, int max_confs, const PMutConfig &curtpl) const {
    if (is_leaf()) {
        if (min_confs <= min_cases_in_one_leaf_ && min_cases_in_one_leaf_ <= max_confs)
            res.emplace_back(new Config(curtpl));
        return;
    }
    CHECK_NE(splitvar, -1);
    for (int v = 0; v < n_childs(); ++v) {
        CHECK(curtpl->get(splitvar) == -1);
        curtpl->set(splitvar, v);
        childs[v]->gather_small_leaves(res, min_confs, max_confs, curtpl);
        curtpl->set(splitvar, -1);
    }
}

void CNode::gather_leaves_nodes(vec<ptr<const CNode>> &res, int min_confs, int max_confs) const {
    if (is_leaf()) {
        if (min_confs <= min_cases_in_one_leaf_ && min_cases_in_one_leaf_ <= max_confs)
            res.emplace_back(this);
        return;
    }
    CHECK_NE(splitvar, -1);
    for (int v = 0; v < n_childs(); ++v) {
        childs[v]->gather_leaves_nodes(res, min_confs, max_confs);
    }
}

void CNode::gen_tpl(Config &conf) const {
    if (is_leaf()) {
        CHECK_EQ(splitvar, -1);
    }
    if (parent != nullptr) {
        CHECK_NE(parent->splitvar, -1);
        conf.set(parent->splitvar, id_);
        parent->gen_tpl(conf);
    }
}

vec<ptr<Config>> CNode::gen_one_convering_configs(int lim) const {
    const PMutContext ctx = tree->ctx();
    const PDomain dom = tree->ctx()->dom();
    //====
    Config templ(ctx);
    templ.set_all(-1);
    gen_tpl(templ);
    //====
    set<hash128_t> shash;
    shash.reserve((size_t) n_total());
    // Note: either hit_configs() or miss_configs() is empty
    CHECK(hit_configs().empty() || miss_configs().empty());
    for (const auto &c : hit_configs()) shash.insert(c->hash_128());
    for (const auto &c : miss_configs()) shash.insert(c->hash_128());
    for (const auto &c : new_configs_) shash.insert(c->hash_128());
    bool use_solver = (shash.size() <= 10000);
    //====
    vec<sm_vec<int>> SetVAL;
    SetVAL.reserve((size_t) dom->n_vars());
    int n_finished = 0;
    for (int i = 0; i < dom->n_vars(); i++) {
        if (templ.get(i) == -1) {
            SetVAL.emplace_back(boost::counting_iterator<int>(0), boost::counting_iterator<int>(dom->n_values(i)));
        } else {
            SetVAL.emplace_back();
            n_finished++;
        }
    }

    VLOG(100, "Tpl: \n") << templ;

    vec<PMutConfig> ret;
    std::unique_ptr<Z3Scope> z3;
    while (n_finished < dom->n_vars() && int(ret.size()) < lim) {
        PMutConfig conf = new Config(ctx);
        sm_vec<int, 128> vrand;
        for (int i = 0; i < dom->n_vars(); i++) {
            sm_vec<int> &st = SetVAL[i];
            int tmplval = templ.values()[i];
            if (tmplval != -1) {
                conf->set(i, tmplval);
            } else if (!st.empty()) {
                auto it = Rand.get(st);
                conf->set(i, *it);
                unordered_erase(st, it);
                if (st.empty())
                    n_finished++;
            } else {
                conf->set(i, Rand.get(dom->n_values(i)));
                vrand.push_back(i);
            }
        }
        if (shash.insert(conf->hash_128()).second) {
            if (z3 != nullptr) (*z3)->add(!conf->to_expr());
            ret.emplace_back(move(conf));
        } else if (use_solver) {
            for (int id : vrand) conf->set(id, -1);

            if (z3 == nullptr) {
                z3 = std::make_unique<Z3Scope>(ctx->zscope());
                for (const auto &c : hit_configs()) (*z3)->add(!c->to_expr());
                for (const auto &c : miss_configs()) (*z3)->add(!c->to_expr());
                for (const auto &c : new_configs_) (*z3)->add(!c->to_expr());
                for (const auto &c : ret) (*z3)->add(!c->to_expr());
            }

            expr e = conf->to_expr();
            if ((*z3)->check(1, &e) == z3::sat) {
                z3::model m = (*z3)->get_model();
                for (int id : vrand)
                    conf->set(id, dom->var(id)->val_id_of(m.get_const_interp(dom->var(id)->zvar().decl())));

                VLOG(100, "Found cex for tpl using solver ({} conds): \n", shash.size()) << templ << '\n' << *conf;
                bool insert_res = shash.insert(conf->hash_128(true /*recalc hash*/)).second;
                CHECK(insert_res) << *conf;
                (*z3)->add(!conf->to_expr());
                ret.emplace_back(move(conf));
            } else {
                VLOG(100, "Can't find cex for tpl: ") << templ;
            }
        }
    }

    VLOG_BLOCK(100, {
        log << "CEX:\n";
        for (const auto &c : ret) log << *c << '\n';
    });

    return ret;
}

}