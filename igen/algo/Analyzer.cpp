//
// Created by kh on 3/14/20.
//

#include "Algo.h"

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/ProgramRunner.h>
#include <igen/CoverageStore.h>

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <igen/c50/CTree.h>
#include <fstream>

namespace igen {


class Analyzer : public Object {
public:
    explicit Analyzer(PMutContext ctx) : Object(move(ctx)) {}

    void run_analyzer_0() {

    }


    void run_alg() {
        switch (ctx()->get_option_as<int>("alg-version")) {
            case 0:
                return run_analyzer_0();
            default:
                CHECK(0);
        }
    }
};


int run_analyzer(const boost::program_options::variables_map &vm) {
    PMutContext ctx = new Context();
    for (const auto &kv : vm) {
        ctx->set_option(kv.first, kv.second.value());
    }
    ctx->init();
    //ctx->program_runner()->init();
    {
        ptr<Analyzer> ite_alg = new Analyzer(ctx);
        ite_alg->run_alg();
    }
    ctx->cleanup();
    return 0;
}


}