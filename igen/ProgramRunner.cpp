//
// Created by KH on 3/7/2020.
//

#include "ProgramRunner.h"

#include <klib/print_stl.h>

#ifdef __MINGW32__
typedef unsigned long DWORD;
#   ifndef __kernel_entry
#       define __kernel_entry
#   endif
#endif

#include <boost/process/child.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/process/args.hpp>

namespace bp = boost::process;

namespace igen {


ProgramRunner::ProgramRunner(PMutContext _ctx) : Object(move(_ctx)), type(RunnerType::Invalid) {
    if (ctx()->has_option("simple-runner")) {
        type = RunnerType::Simple;
    } else {
        str str_type = ctx()->get_option_as<str>("runner");
        type = RunnerType::_from_string_nocase(str_type.c_str());
    }
    if (ctx()->has_option("target")) {
        target = ctx()->get_option_as<str>("target");
    } else {
        target = ctx()->get_option_as<str>("filestem") + ".exe";
    }
}

set<str> ProgramRunner::run(const PConfig &config) const {
    switch (type) {
        case RunnerType::Simple:
            return _run_simple(config);
        default:
            CHECK(0);
    }
}

set<str> ProgramRunner::_run_simple(const PConfig &config) const {
    bp::ipstream out, err;
    vec<str> args = config->value_labels();
    bp::child proc_child(target, bp::args(args), bp::std_out > out, bp::std_err > err);
    CHECKF(proc_child, "Error running process {}, args {}", target, args);

    set<str> locations;
    str loc;
    while (getline(out, loc)) {
        if (loc.size() && loc.back() == '\r')
            loc.pop_back();
        locations.insert(loc);
    }

    std::string serr(std::istreambuf_iterator<char>(err), {});
    if (serr.size()) {
        LOG(WARNING, "cerr is not empty: ") << err.rdbuf();
    }

    return locations;
}

void intrusive_ptr_release(ProgramRunner *p) {
    intrusive_ptr_add_ref(p);
}

}