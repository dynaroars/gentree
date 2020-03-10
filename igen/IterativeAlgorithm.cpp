//
// Created by KH on 3/5/2020.
//

#include "IterativeAlgorithm.h"

#include "Context.h"
#include "Domain.h"
#include "Config.h"
#include "ProgramRunner.h"
#include "CoverageStore.h"

#include <klib/print_stl.h>
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
        auto configs = dom()->gen_one_convering_configs();
        VLOG(10, "gen & run one convering configs");
        for (const auto &c : configs) {
            auto e = ctx()->program_runner()->run(c);
            cov()->register_cov(c, e);
            VLOG(20, "{}  ==>  ", *c) << e;
        }
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