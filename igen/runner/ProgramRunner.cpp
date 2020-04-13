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

#define BOOST_POSIX_HAS_VFORK 1

#include <boost/process/child.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/process/args.hpp>
#include <boost/process/posix.hpp>

#include <boost/scope_exit.hpp>

#include "GCovRunner.h"

namespace bp = boost::process;

namespace igen {


ProgramRunner::ProgramRunner(PMutContext _ctx, map<str, str> default_vars) :
        Object(move(_ctx)),
        type(RunnerType::Invalid), default_vars_(move(default_vars)) {

    timer_.stop();

    if (ctx()->has_option("simple-runner")) {
        type = RunnerType::Simple;
    } else if (ctx()->has_option("builtin-runner")) {
        type = RunnerType::BuiltIn;
    } else if (ctx()->has_option("gcov-runner")) {
        type = RunnerType::GCov;
    } else if (ctx()->has_option("otter-runner")) {
        type = RunnerType::Otter;
    } else {
        str str_type = ctx()->get_option_as<str>("runner");
        type = RunnerType::_from_string_nocase(str_type.c_str());
    }
    if (ctx()->has_option("target")) {
        target = ctx()->get_option_as<str>("target");
    } else {
        str filestem = ctx()->get_option_as<str>("filestem");
        if (type == +RunnerType::GCov)
            target = filestem + ".gcov";
        else if (type == +RunnerType::Otter)
            target = filestem + ".otter";
        else
            target = filestem + ".exe";
    }
    // LOG(INFO, "Program runner: type {}, target {}", type._to_string(), target);
    if (type == +RunnerType::BuiltIn) {
        builtin_fn = builtin::get_fn(target);
        // LOG(INFO, "Builtin runner source: ") << builtin::get_src(target);
    }
    if (type == +RunnerType::GCov) {
        gcov_runner_ = new GCovRunner(ctx_mut(), default_vars_);
        gcov_runner_->parse(target);
    }
    if (type == +RunnerType::Otter) {
        otter_runner_ = OtterRunner::create(target);
    }
}

set<str> ProgramRunner::run(const PConfig &config) {
    timer_.resume();
    BOOST_SCOPE_EXIT(this_) { this_->timer_.stop(); }
    BOOST_SCOPE_EXIT_END

    set<str> ret;
    n_runs_++;
    switch (type) {
        case RunnerType::Simple:
            ret = _run_simple(config);
            break;
        case RunnerType::BuiltIn:
            ret = _run_builtin(config);
            break;
        case RunnerType::GCov:
            ret = _run_gcov(config);
            break;
        case RunnerType::Otter:
            ret = _run_otter(config);
            break;
        default:
            CHECK(0);
    }
    return ret;
}

set<str> ProgramRunner::_run_simple(const PConfig &config) const {
    bp::ipstream out, err;
    vec<str> args = config->value_labels();
    bp::child proc_child(target, bp::posix::use_vfork, bp::posix::sig.ign(),
                         bp::args(args), bp::std_out > out, bp::std_err > err);
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

set<str> ProgramRunner::_run_builtin(const PConfig &config) const {
    return builtin_fn(config);
}

set<str> ProgramRunner::_run_gcov(const PConfig &config) const {
    vec<str> str_args;
    str_args.reserve(size_t(config->ctx()->dom()->n_vars()) * 2);
    for (const auto &e : *config) {
        const str &name = e.name(), &label = e.label();
        bool on = label == "on";
        bool off = label == "off";
        if (on || off) {
            if (on) str_args.emplace_back(name);
        } else {
            if (name == "+") {
                str tmp = name;
                tmp += label;
                str_args.emplace_back(move(tmp));
            } else if (name.size() > 2 && name[0] == '-' && name[1] == '-') {
                str tmp = name;
                tmp += '=', tmp += label;
                str_args.emplace_back(move(tmp));
            } else {
                CHECKF(0, "Invalid name/lable pair: {} {}", name, label);
                str_args.emplace_back(name);
                str_args.emplace_back(label);
            }
        }
    }
    gcov_runner_->clean_cov();
    gcov_runner_->exec(str_args);
    return gcov_runner_->collect_cov();
}

void ProgramRunner::cleanup() {
    gcov_runner_.reset();
}

void ProgramRunner::init() {
    if (gcov_runner_ != nullptr) gcov_runner_->init();
}

int ProgramRunner::n_locs() const {
    if (gcov_runner_ != nullptr) return gcov_runner_->n_locs();
    if (otter_runner_ != nullptr) return otter_runner_->n_locs();
    return -1;
}

set<str> ProgramRunner::_run_otter(const PConfig &config) const {
    CHECK_NE(otter_runner_, nullptr);
    return otter_runner_->run(config);
}

}