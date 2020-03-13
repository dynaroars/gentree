//
// Created by KH on 3/5/2020.
//

#include "IterAlgo.h"

#include "Context.h"
#include "Domain.h"
#include "Config.h"
#include "ProgramRunner.h"
#include "CoverageStore.h"

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <igen/c50/CTree.h>
#include <fstream>

namespace igen {

class IterativeAlgorithm : public Object {
public:
    explicit IterativeAlgorithm(PMutContext ctx) : Object(move(ctx)) {}

    ~IterativeAlgorithm() override = default;

    void run_alg_full() {
        str loc = ctx()->get_option_as<str>("loc");
        auto configs = dom()->gen_all_configs();
        for (const auto &c : configs) {
            auto e = ctx()->program_runner()->run(c);
            cov()->register_cov(c, e);
            //VLOG(20, "{}  ==>  ", *c) << e;
            // VLOG_BLOCK(21, {
            //     for (const auto &x : c->cov_locs()) {
            //         log << x->name() << ", ";
            //     }
            // });
        }

        PMutCTree tree = new CTree(ctx());
        tree->prepare_data_for_loc(cov()->loc(loc));
        tree->build_tree();
        LOG(INFO, "DECISION TREE = \n") << (*tree);
        z3::expr e = tree->build_zexpr(CTree::DisjOfConj);
        LOG(INFO, "EXPR BEFORE = \n") << e.simplify();
        //z3::params simpl_params(ctx()->zctx());
        //simpl_params.set("rewrite_patterns", true);
        e = ctx()->zctx_solver_simplify(e);
        LOG(INFO, "EXPR AFTER = \n") << e;
        //LOG(INFO, "SOLVER = \n") << ctx()->zsolver();
        for (const auto &c : cov()->configs()) {
            bool val = c->eval(e);
            //LOG(INFO, "{} ==> {}", *c, val);
            int should_be = c->cov_loc("L1");
            CHECK_EQ(val, should_be);
        }
        LOG(INFO, "Verified expr with {} configs", cov()->n_configs());
    }

    void run_alg_test_0() {
        str loc = ctx()->get_option_as<str>("loc");

        //=== TEST ALL CONF =================================================================
        PMutConfig ALL_CONF_TPL = new Config(ctx_mut());
        ALL_CONF_TPL->set_all(-1);
        for (int i = 8; i < dom()->n_vars(); i++)
            ALL_CONF_TPL->set(i, 0);
        auto all_configs = dom()->gen_all_configs(ALL_CONF_TPL);
        //vec<bool> all_configs_res;
        for (int i = 0; i < int(all_configs.size()); ++i) {
            auto e = ctx()->program_runner()->run(all_configs[i]);
            all_configs[i]->set_id(e.contains(loc));
            //all_configs_res.push_back(e.contains(loc));
        }
        ctx_mut()->program_runner()->reset_n_runs(0);
        //=== END TEST ALL CONF =================================================================

        set<hash128_t> set_conf_hash;

        auto configs = dom()->gen_one_convering_configs();
        int n_one_covering = int(configs.size());
        auto seed_configs = get_seed_configs();
        vec_move_append(configs, seed_configs);
        VLOG(10, "Run initial configs (n_one_covering = {})", n_one_covering);
        for (const auto &c : configs) {
            auto e = ctx()->program_runner()->run(c);
            cov()->register_cov(c, e);
            set_conf_hash.insert(c->hash_128());
            VLOG(20, "{}  ==>  ", *c) << e;
        }

        PMutCTree tree = new CTree(ctx());
        {
            tree->prepare_data_for_loc(cov()->loc(loc));
            tree->build_tree();
            LOG(INFO, "INITIAL DECISION TREE = \n") << (*tree);
        }

        int N_ROUNDS = ctx()->get_option_as<int>("rounds");
        for (int iteration = 0; iteration < N_ROUNDS; ++iteration) {
            LOG(INFO, "{:=^80}", fmt::format("  Iteration {}  ", iteration));


            //LOG(INFO, "n_min_cases_in_one_leaf = {}", tree->n_min_cases_in_one_leaf());
            vec<PMutConfig> cex;
            int lim_gather = tree->n_min_cases_in_one_leaf(), prev_lim_gather = -1;
            int skipped = 0, cex_tried = 0;
            while (cex.empty()) {
                vec<PConfig> tpls = tree->gather_small_leaves(prev_lim_gather + 1, lim_gather);
                LOG_BLOCK(INFO, {
                    fmt::print(log, "Tpls =  (lim {} -> {})\n", prev_lim_gather + 1, lim_gather);
                    for (const auto &c : tpls) log << *c << "\n";
                });

                for (const auto &t : tpls) {
                    vec<PMutConfig> vp = dom()->gen_one_convering_configs(t);
                    //vec_move_append(cex, vp);
                    for (auto &c : vp) {
                        if (!set_conf_hash.insert(c->hash_128()).second)
                            skipped++;
                        else
                            cex.emplace_back(move(c));
                    }
                }
                prev_lim_gather = lim_gather;
                lim_gather *= 2;
                CHECK_LE(lim_gather, int(1e9));
                if (++cex_tried == 2) break;
            }
            LOG(INFO, "Skipped {} duplicated configs", skipped);
            while (cex.empty()) {
                LOG(WARNING, "Can't gen cex, try random configs");
                const auto &vp = dom()->gen_one_convering_configs();
                for (auto &c : vp) {
                    if (set_conf_hash.insert(c->hash_128()).second)
                        cex.emplace_back(move(c));
                }
                if (++cex_tried == 30) {
                    LOG(ERROR, "Can't gen cex anymore");
                    goto end_alg;
                }
            }

//            LOG_BLOCK(INFO, {
//                log << "New CEXs = \n";
//                for (const auto &c : cex) log << *c << "\n";
//            });

            for (const auto &c : cex) {
                auto e = ctx()->program_runner()->run(c);
                cov()->register_cov(c, e);
                VLOG(20, "{}  ==>  ", *c) << e;
            }



            // =============================


            tree->cleanup();
            tree->prepare_data_for_loc(cov()->loc(loc));
            tree->build_tree();
            LOG(INFO, "DECISION TREE = \n") << (*tree);
            z3::expr e = tree->build_zexpr(CTree::DisjOfConj);
            e = e.simplify();
            //e = ctx()->zctx_solver_simplify(e);
            //LOG(INFO, "EXPR AFTER = \n") << e;


            // ==== TEST=================
            int n_wrongs = 0;
            vec<PConfig> vwrongs;
            for (const auto &c : all_configs) {
                bool c_eval = c->eval(e);
                bool truth = c->id();
                bool tree_eval = tree->test_config(c).first;
                CHECK_EQ(c_eval, tree_eval);
                bool wr = (c_eval != truth);
                n_wrongs += wr;
                if (wr && vwrongs.size() < 10) vwrongs.push_back(c);
            }
            LOG(INFO, "VERIFY error {}/{} ( {:.1f}% )",
                n_wrongs, all_configs.size(),
                (n_wrongs * 100.0) / double(all_configs.size()));
            if (n_wrongs <= 10) {
                LOG_BLOCK(INFO, {
                    log << "Wrong configs:\n";
                    for (const auto &c : vwrongs) log << *c << '\n';
                });
            }
//            if (n_wrongs == 0 && N_ROUNDS - iteration > 5) {
//                N_ROUNDS = iteration + 5 + 1;
//                LOG(WARNING, "Early cut to {} interations", N_ROUNDS);
//            }
            // ==== END TEST ============
        }

        end_alg:
        LOG(INFO, "Runner n_runs = {}", ctx()->program_runner()->n_runs());
    }


#define TREE_DATA(id) auto &[tree, shared_tree, tree_need_rebuild] = vec_loc_data.at(size_t(id))

    void finalize_build_trees() {
        LOG(INFO, "finalize_build_trees");
        map_loc_hash.clear();
        int n_min_cases_in_one_leaf = std::numeric_limits<int>::max();
        int n_rebuilds = 0;
        for (const PLocation &loc : cov()->locs()) {
            if (loc->id() >= int(vec_loc_data.size())) break;
            PLocation &mloc = map_loc_hash[loc->digest_cov_by_hash()];
            TREE_DATA(loc->id());

            if (mloc == nullptr) {
                shared_tree = nullptr;
                mloc = loc;
                VLOG(20, "Process loc {}: {}", loc->id(), loc->name());
            } else {
                tree = nullptr;
                shared_tree = vec_loc_data.at(size_t(mloc->id())).tree;
                //VLOG(100, "Duplicated loc {}: {} (<=> {}: {})", loc->id(), loc->name(), mloc->id(), mloc->name());
                continue;
            }

            tree = new CTree(ctx()), tree->prepare_data_for_loc(loc), tree->build_tree();
            VLOG(30, "DECISION TREE (n_min_cases = {}) = \n", tree->n_min_cases_in_one_leaf()) << (*tree);
            n_min_cases_in_one_leaf = std::min(n_min_cases_in_one_leaf, tree->n_min_cases_in_one_leaf());
            n_rebuilds++;
        }
        LOG(INFO, "(END finalize_build_trees) unique locations = {}, n_min_cases_in_one_leaf = {}, n_unvisited = {}",
            n_rebuilds, n_min_cases_in_one_leaf, int(vec_loc_data.size()) - cov()->n_locs());
    }

    set<hash128_t> set_conf_hash, set_ran_conf_hash;
    map<hash128_t, PLocation> map_loc_hash;
    struct LocData {
        PMutCTree tree;
        PCTree shared_tree;
        bool tree_need_rebuild;
    };
    vec<LocData> vec_loc_data;

    bool alg_test_1_iteration([[maybe_unused]] int iter) {
        vec_loc_data.resize(size_t(cov()->n_locs()));

        vec<PCNode> leaves;
        int n_min_cases_in_one_leaf = std::numeric_limits<int>::max();
        map_loc_hash.clear();
        for (const PLocation &loc : cov()->locs()) {
            PLocation &mloc = map_loc_hash[loc->digest_cov_by_hash()];
            TREE_DATA(loc->id());

            if (mloc == nullptr) {
                shared_tree = nullptr;
                mloc = loc;
                VLOG(20, "Process loc {}: {}", loc->id(), loc->name());
            } else {
                tree = nullptr;
                shared_tree = vec_loc_data.at(size_t(mloc->id())).tree;
                VLOG(100, "Duplicated loc {}: {} (<=> {}: {})", loc->id(), loc->name(), mloc->id(), mloc->name());
                continue;
            }

            if (tree == nullptr) {
                tree = new CTree(ctx()), tree->prepare_data_for_loc(loc), tree->build_tree();
                VLOG(30, "NEW DECISION TREE (n_min_cases = {}) = \n", tree->n_min_cases_in_one_leaf()) << (*tree);
            }

            n_min_cases_in_one_leaf = std::min(n_min_cases_in_one_leaf, tree->n_min_cases_in_one_leaf());
            tree->gather_leaves_nodes(leaves);
        }
        // ============================================================================================================

        std::sort(leaves.begin(), leaves.end(), [](const PCNode &a, const PCNode &b) {
            return a->min_cases_in_one_leaf() < b->min_cases_in_one_leaf();
        });

        vec<PMutConfig> cex;
        PMutConfig gen_tpl = new Config(ctx_mut());
        int skipped = 0;
        for (const PCNode &node : leaves) {
            if (node->min_cases_in_one_leaf() > 0 && cex.size() > 20) break;
            gen_tpl->set_all(-1);
            node->gen_tpl(gen_tpl);
            VLOG(30, "Tpl ({}): ", node->min_cases_in_one_leaf()) << *gen_tpl;

            for (auto &c : dom()->gen_one_convering_configs(gen_tpl)) {
                if (set_conf_hash.insert(c->hash_128()).second) cex.emplace_back(move(c));
                else skipped++;
            }
        }
        VLOG_IF(25, skipped, "Skipped {} duplicated configs", skipped);

        // === Try rand
        int cex_rand_tried = 0;
        while (cex.empty()) {
            LOG(WARNING, "Can't gen cex, try random configs");
            const auto &vp = dom()->gen_one_convering_configs();
            for (auto &c : vp) {
                if (set_conf_hash.insert(c->hash_128()).second) cex.emplace_back(move(c));
            }
            if (++cex_rand_tried == 30) break;
        }

        LOG(INFO, "Generated {} cex", cex.size());
//        LOG_BLOCK(INFO, {
//            log << "CEX:\n";
//            for (const auto &c : cex) log << *c << '\n';
//        });
        if (cex.empty()) return false;

        // ============================================================================================================

        for (const auto &c : cex) run_config(c);

        int n_new_locs = cov()->n_locs() - int(vec_loc_data.size());
        int n_rebuilds = 0;
        for (const PMutConfig &c : cex) {
            const vec<int> cov_ids = c->cov_loc_ids();
            auto it = cov_ids.begin();
            for (const PLocation &loc : cov()->locs()) {
                if (loc->id() >= int(vec_loc_data.size())) break;
                TREE_DATA(loc->id());
                if (tree == nullptr && shared_tree == nullptr) continue;

                bool new_truth = false;
                if (it != cov_ids.end() && *it == loc->id()) new_truth = true, ++it;
                bool tree_eval = (tree != nullptr ?
                                  tree->test_add_config(c, new_truth).first : shared_tree->test_config(c).first);

                tree_need_rebuild = (tree_eval != new_truth);
                if (tree_need_rebuild) {
                    n_rebuilds++;
                    tree = nullptr, shared_tree = nullptr;
                    VLOG(20, "Need rebuild loc {}", loc->name());
                    continue;
                }
            }
        }

        LOG(INFO, "(END ITER) n_rebuilds = {}, n_new_locs = {}, n_min_cases_in_one_leaf = {}",
            n_rebuilds, n_new_locs, n_min_cases_in_one_leaf);
        bool need_term = n_rebuilds == 0 && n_new_locs == 0 && n_min_cases_in_one_leaf > 0;
        LOG(WARNING, "need_term = TRUE, terminate_counter = {}", terminate_counter);
        if (need_term) {
            if (++terminate_counter == 10) return false;
        } else {
            terminate_counter = 0;
        }
        return true;
    }

    int terminate_counter;

    void run_alg_test_1() {

        auto init_configs = dom()->gen_one_convering_configs();
        {
            int n_one_covering = int(init_configs.size());
            auto seed_configs = get_seed_configs();
            int n_seed_configs = int(seed_configs.size());
            vec_move_append(init_configs, seed_configs);
            VLOG(10, "Run initial configs (n_one_covering = {}, n_seed_configs = {})", n_one_covering, n_seed_configs);
        }

        for (const auto &c : init_configs) set_conf_hash.insert(c->hash_128()), run_config(c);

        int N_ROUNDS = ctx()->get_option_as<int>("rounds");
        for (int iter = 1; iter <= N_ROUNDS; ++iter) {
            LOG(INFO, "{:=^80}", fmt::format("  Iteration {}  ", iter));
            if (!alg_test_1_iteration(iter)) {
                LOG(WARNING, "Early break at iteration {}", iter);
                break;
            }
        }

        finalize_build_trees();

        finish_alg1();
    }

    void finish_alg1() {
        bool out_to_file = false;
        std::ofstream outstream;
        if (ctx()->has_option("output")) {
            str fout = ctx()->get_option_as<str>("output");
            outstream.open(fout);
            out_to_file = true;
            CHECKF(!outstream.fail(), "Can't open output file {}", fout);
        }

        struct LineEnt {
            vec<PLocation> locs;
            str expr;
        };
        vec<LineEnt> ents;
        if (out_to_file) ents.resize(vec_loc_data.size());

        LOG(INFO, "{:=^80}", "  FINAL RESULT  ");
        map_loc_hash.clear();
        for (const PLocation &loc : cov()->locs()) {
            if (loc->id() >= int(vec_loc_data.size())) break;
            std::stringstream log;
            TREE_DATA(loc->id());
            PLocation &mloc = map_loc_hash[loc->digest_cov_by_hash()];
            fmt::print(log, "{:>5}: {:<16}  ==>  ", loc->id(), loc->name());
            if (mloc == nullptr) {
                CHECK_NE(tree, nullptr);
                mloc = loc;
                z3::expr e = tree->build_zexpr(CTree::DisjOfConj);
                e = e.simplify();
                e = ctx()->zctx_solver_simplify(e);

                if (out_to_file) {
                    auto &et = ents.at(size_t(loc->id()));
                    et.locs.push_back(loc);
                    et.expr = e.to_string();
                    log << et.expr;
                } else {
                    log << e;
                }
            } else {
                CHECK_NE(shared_tree, nullptr);
                fmt::print(log, "{}: {}", mloc->id(), mloc->name());
                if (out_to_file) ents.at(size_t(mloc->id())).locs.push_back(loc);
            }
            GLOG(INFO) << log.rdbuf();
        }
        LOG(INFO, "Runner n_runs = {}", ctx()->program_runner()->n_runs());

        for (const auto &e : ents) {
            if (e.locs.empty()) continue;
            for (const auto &loc : e.locs)
                outstream << loc->name() << ", ";
            outstream << '\n' << e.expr << "\n===\n";
        }
    }

    void run_config(const PMutConfig &c) {
        auto e = ctx()->program_runner()->run(c);
        cov()->register_cov(c, e);
        bool insert_new = set_ran_conf_hash.insert(c->hash_128()).second;
        CHECK(insert_new);
        if (dom()->n_vars() <= 16 && e.size() <= 16) {
            VLOG(50, "{}  ==>  ", *c) << e;
        } else {
            VLOG(50, "Config {}  ==>  {} locs", c->id(), e.size());// << e;
        }
    }

    void run_alg() {
        if (ctx()->has_option("full")) {
            run_alg_full();
        } else if (ctx()->has_option("alg-test")) {
            int v = ctx()->get_option_as<int>("alg-test");
            switch (v) {
                case 0:
                    return run_alg_test_0();
                case 1:
                    return run_alg_test_1();
                default:
                    CHECK(0);
            }
        }
    }

    vec<PMutConfig> get_seed_configs() const {
        vec<PMutConfig> ret;
        if (ctx()->has_option("seed-configs")) {
            vec<str> vs = ctx()->get_option_as<vec<str>>("seed-configs");
            for (const auto &s : vs)
                ret.emplace_back(new Config(ctx_mut(), s));
        }
        return ret;
    }

private:
};

int run_interative_algorithm(const boost::program_options::variables_map &vm) {
    PMutContext ctx = new Context();
    for (const auto &kv : vm) {
        ctx->set_option(kv.first, kv.second.value());
    }
    ctx->init();
    {
        ptr<IterativeAlgorithm> ite_alg = new IterativeAlgorithm(ctx);
        ite_alg->run_alg();
    }
    ctx->cleanup();
    return 0;
}


}