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

    ~IterativeAlgorithm() = default;

    void run_alg() {
        auto configs = dom()->gen_all_configs();
        for (const auto &c : configs) {
            auto e = ctx()->program_runner()->run(c);
            cov()->register_cov(c, e);
            VLOG(20, "{}  ==>  ", *c) << e;
            // VLOG_BLOCK(21, {
            //     for (const auto &x : c->cov_locs()) {
            //         log << x->name() << ", ";
            //     }
            // });
        }

        PMutCTree tree = new CTree(ctx());
        tree->prepare_data_for_loc(cov()->loc("L1"));
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