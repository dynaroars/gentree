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
        z3solver_(z3ctx_), z3true_(z3ctx_.bool_val(true)), z3false_(z3ctx_.bool_val(false)),
        z3simplifier_(z3ctx_, "ctx-solver-simplify") {
    constexpr int MAX_TIME = 60000;

    z3::context &ctx = const_cast<Context *>(this)->zctx();
    z3::params params(ctx);
    params.set("random_seed", 456123u);
    params.set(":random_seed", 456123u);
    z3simplifier_ = z3::with(z3simplifier_, params);
    z3simplifier_ = z3::try_for(z3simplifier_, MAX_TIME);
    z3simplifier_ = z3::repeat(z3simplifier_);
    z3simplifier_ = z3::try_for(z3simplifier_, MAX_TIME);

    z3solver_.set("random-seed", 123456u);
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
        LOG(WARNING, "Use shared program runner");
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
    auto _this = const_cast<Context *>(this);
    z3::goal goal(_this->z3ctx_);
    expr simple_simpl = e.simplify();
    try {
        goal.add(simple_simpl);
        z3::apply_result tatic_result = _this->z3simplifier_.apply(goal);
        z3::goal result_goal = tatic_result[0];
        return result_goal.as_expr();
    } catch (z3::exception &e) {
        LOG(WARNING, "zctx_solver_simplify exception: ") << e;
        return simple_simpl;
    }
}

}