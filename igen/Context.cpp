//
// Created by KH on 3/5/2020.
//

#include "Context.h"
#include "Domain.h"
#include "ProgramRunnerMt.h"
#include "CoverageStore.h"

#include <fstream>

namespace igen {

Context::Context() :
        z3solver_(z3ctx_), z3true_(z3ctx_.bool_val(true)), z3false_(z3ctx_.bool_val(false)) {
    z3solver_.set(":random-seed", 123456u);
}

Context::~Context() {
    LOG(INFO, "Context destroyed");
}

void Context::set_option(const str &key, boost::any val) {
    options[key] = val;
}

void Context::set_options(map<str, boost::any> opts) {
    options = std::move(opts);
}

bool Context::has_option(const str &key) const {
    return options.contains(key);
}

boost::any Context::get_option(const str &key) const {
    auto it = options.find(key);
    CHECKF(it != options.end(), "Missing option \"{}\"", key);
    return it->second;
}

void Context::init() {
    shared_program_runner_ = has_option("_shared_program_runner");
    c_dom_ = dom_ = new Domain(this);
    if (shared_program_runner_) {
        program_runner_ = get_option_as<PMutProgramRunnerMt>("_shared_program_runner");
    } else {
        program_runner_ = new ProgramRunnerMt(this);
    }
    CHECK_NE(program_runner_, nullptr);
    c_coverage_store_ = coverage_store_ = new CoverageStore(this);
}

void Context::init_runner() {
    if (!shared_program_runner_) {
        program_runner_->init();
    }
}

void Context::cleanup() {
    coverage_store_->cleanup(), coverage_store_ = nullptr, c_coverage_store_ = nullptr;
    if (shared_program_runner_) {
        program_runner_->flush_cachedb();
    } else {
        program_runner_->cleanup();
    }
    program_runner_ = nullptr;
    dom_->cleanup(), dom_ = nullptr, c_dom_ = nullptr;
}

expr Context::zctx_solver_simplify(const expr &e) const {
    constexpr int MAX_TIME = 30000;

    z3::context &ctx = const_cast<Context *>(this)->zctx();
    z3::tactic tactic(ctx, "ctx-solver-simplify");
    tactic = z3::try_for(tactic, MAX_TIME);
    tactic = z3::repeat(tactic);
    tactic = z3::try_for(tactic, MAX_TIME);
    z3::goal goal(ctx);
    expr simple_simpl = e.simplify();
    try {
        goal.add(simple_simpl);
        z3::apply_result tatic_result = tactic.apply(goal);
        z3::goal result_goal = tatic_result[0];
        return result_goal.as_expr();
    } catch (z3::exception &e) {
        LOG(WARNING, "zctx_solver_simplify exception: ") << e;
        return simple_simpl;
    }
}

}