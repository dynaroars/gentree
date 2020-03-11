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

    void run_alg_test() {
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
            if (cex.empty()) {
                LOG(WARNING, "Can't gen cex, try random configs");
                cex = dom()->gen_one_convering_configs();
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
            for (const auto &c : all_configs) n_wrongs += (c->eval(e) != c->id());
            LOG(INFO, "VERIFY error {}/{} ( {:.1f}% )",
                n_wrongs, all_configs.size(),
                (n_wrongs * 100.0) / double(all_configs.size()));
            if (n_wrongs == 0 && N_ROUNDS - iteration > 2) {
                N_ROUNDS = iteration + 2 + 1;
                LOG(WARNING, "Early cut to {} interations", N_ROUNDS);
            }
            // ==== END TEST ============
        }

        LOG(INFO, "Runner n_runs = {}", ctx()->program_runner()->n_runs());
    }

    void run_alg() {
        if (ctx()->has_option("full")) {
            run_alg_full();
        } else if (ctx()->has_option("alg-test")) {
            run_alg_test();
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