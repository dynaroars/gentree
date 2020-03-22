//
// Created by KH on 3/5/2020.
//

#include "Context.h"
#include "Domain.h"
#include "ProgramRunner.h"
#include "CoverageStore.h"

#include <fstream>

namespace igen {

Context::Context() :
        z3solver_(z3ctx_), z3true_(z3ctx_.bool_val(true)), z3false_(z3ctx_.bool_val(false)) {
}

Context::~Context() {
    LOG(INFO, "Context destroyed");
}

void Context::set_option(const str &key, boost::any val) {
    options[key] = val;
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
    cdom_ = dom_ = new Domain(this);
    program_runner_ = new ProgramRunner(this);
    coverage_store_ = new CoverageStore(this);
}

void Context::cleanup() {
    coverage_store_->cleanup(), coverage_store_ = nullptr;
    program_runner_->cleanup(), program_runner_ = nullptr;
    dom_->cleanup(), dom_ = nullptr, cdom_ = nullptr;
}

expr Context::zctx_solver_simplify(const expr &e) const {
    z3::context &ctx = const_cast<Context *>(this)->zctx();
    z3::tactic tactic(ctx, "ctx-solver-simplify");
    tactic = z3::repeat(tactic);
    tactic = z3::try_for(tactic, 20000);
    z3::goal goal(ctx);
    expr simple_simpl = e.simplify();
    goal.add(simple_simpl);
    z3::apply_result tatic_result = tactic.apply(goal);
    z3::goal result_goal = tatic_result[0];
    try {
        return result_goal.as_expr();
    } catch (z3::exception &e) {
        LOG(WARNING, "zctx_solver_simplify exception: ") << e;
        return simple_simpl;
    }
}

}