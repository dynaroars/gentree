//
// Created by KH on 3/5/2020.
//

#include "AlgoDriver.h"

#include "Context.h"

namespace igen {


int run_interative_algorithm(const boost::program_options::variables_map &vm) {
    PContext ctx = new Context();
    ctx->set_option("filestem", vm["filestem"].as<str>());
    ctx->init();
    return 0;
}


}