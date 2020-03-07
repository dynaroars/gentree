//
// Created by KH on 3/5/2020.
//

#include "AlgoDriver.h"

#include "Context.h"
#include "Domain.h"

namespace igen {


int run_interative_algorithm(const boost::program_options::variables_map &vm) {
    PMutContext ctx = new Context();
    ctx->set_option("filestem", vm["filestem"].as<str>());
    ctx->init();
    LOG(INFO, "{}", *ctx->dom());
    return 0;
}


}