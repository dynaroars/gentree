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
        auto all_configs = dom()->gen_all_configs();
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
        VLOG(10, "gen & run one convering configs");
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
            vec<PConfig> tpls = tree->gather_small_leaves(tree->n_min_cases_in_one_leaf());
            LOG_BLOCK(INFO, {
                log << "Tpls = \n";
                for (const auto &c : tpls) log << *c << "\n";
            });

            vec<PMutConfig> cex;
            for (const auto &t : tpls) {
                vec<PMutConfig> vp = dom()->gen_one_convering_configs(t);
                vec_move_append(cex, vp);
            }
//            LOG_BLOCK(INFO, {
//                log << "New CEXs = \n";
//                for (const auto &c : cex) log << *c << "\n";
//            });

            int skipped = 0;
            for (const auto &c : cex) {
                if (!set_conf_hash.insert(c->hash_128()).second) {
                    skipped++;
                    continue;
                }
                auto e = ctx()->program_runner()->run(c);
                cov()->register_cov(c, e);
                VLOG(20, "{}  ==>  ", *c) << e;
            }
            LOG(INFO, "Skipped {} duplicated configs", skipped);



            // =============================


            tree->cleanup();
            tree->prepare_data_for_loc(cov()->loc(loc));
            tree->build_tree();
            LOG(INFO, "DECISION TREE = \n") << (*tree);
            z3::expr e = tree->build_zexpr(CTree::DisjOfConj);
            e = ctx()->zctx_solver_simplify(e);
            LOG(INFO, "EXPR AFTER = \n") << e;


            // ==== TEST=================
            int n_wrongs = 0;
            for (const auto &c : all_configs) n_wrongs += (c->eval(e) != c->id());
            LOG(INFO, "VERIFY error {}/{} ( {:.1f}% )",
                n_wrongs, all_configs.size(),
                (n_wrongs * 100.0) / double(all_configs.size()));
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