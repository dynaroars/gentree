//
// Created by KH on 3/5/2020.
//

#include "AlgoDriver.h"

#include "Context.h"
#include "Domain.h"
#include "Config.h"

#include <klib/print_stl.h>

namespace igen {

class IterativeAlgorithm : public Object {
public:
    explicit IterativeAlgorithm(PMutContext ctx) : Object(move(ctx)) {}
    ~IterativeAlgorithm() = default;

    void run_alg() {
        auto configs = ctx()->dom()->gen_all_configs();
        LOG(INFO, "{}", configs);
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