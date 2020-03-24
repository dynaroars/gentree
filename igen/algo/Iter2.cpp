//
// Created by kh on 3/23/20.
//

#include "Algo.h"

#include <fstream>
#include <csignal>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/circular_buffer.hpp>

#include <igen/Context.h>
#include <igen/Domain.h>
#include <igen/Config.h>
#include <igen/ProgramRunner.h>
#include <igen/CoverageStore.h>
#include <igen/c50/CTree.h>

#include <klib/print_stl.h>
#include <klib/vecutils.h>
#include <klib/random.h>
#include <glog/raw_logging.h>

namespace igen {

namespace {
static volatile std::sig_atomic_t gSignalStatus = 0;
}

class Iter2 : public Object {
public:
    explicit Iter2(PMutContext ctx) : Object(move(ctx)) {}

    ~Iter2() override = default;

    void run_alg() {

    }
};

int run_interative_algorithm_2(const boost::program_options::variables_map &vm) {
    std::signal(SIGINT, [](int signal) {
        if (gSignalStatus) {
            RAW_LOG(ERROR, "Force terminate");
            exit(1);
        }
        gSignalStatus = signal;
    });

    PMutContext ctx = new Context();
    for (const auto &kv : vm) {
        ctx->set_option(kv.first, kv.second.value());
    }
    ctx->init();
    ctx->program_runner()->init();
    {
        ptr<Iter2> ite_alg = new Iter2(ctx);
        ite_alg->run_alg();
    }
    ctx->cleanup();
    return 0;
}

}