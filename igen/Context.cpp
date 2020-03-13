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
    VLOG(1, "Context destroyed");
}

void Context::set_option(const str &key, boost::any val) {
    options[key] = val;
}

bool Context::has_option(const str &key) const {
    return options.contains(key);
}

boost::any Context::get_option(const str &key) const {
    auto it = options.find(key);
    CHECK(it != options.end());
    return it->second;
}

void Context::init() {
    dom_ = new Domain(this);
    program_runner_ = new ProgramRunner(this), program_runner_->init();
    coverage_store_ = new CoverageStore(this);
}

void Context::cleanup() {
    coverage_store_->cleanup(), coverage_store_ = nullptr;
    program_runner_->cleanup(), program_runner_ = nullptr;
    dom_->cleanup(), dom_ = nullptr;
}


ptr<const Domain> Context::dom() const { return dom_; }

const ptr<Domain> &Context::dom() { return dom_; }

ptr<const Domain> Object::dom() const { return ctx_->dom(); }

ptr<const CoverageStore> Object::cov() const { return ctx_->cov(); }

ptr<CoverageStore> Object::cov() { return ctx_->cov(); }

ptr<const ProgramRunner> Context::program_runner() const { return program_runner_; }

const ptr<ProgramRunner> &Context::program_runner() { return program_runner_; }

ptr<const CoverageStore> Context::cov() const { return coverage_store_; }

const ptr<CoverageStore> &Context::cov() { return coverage_store_; }

expr Context::zctx_solver_simplify(const expr &e) const {
    z3::context &ctx = const_cast<Context *>(this)->zctx();
    z3::tactic tactic(ctx, "ctx-solver-simplify");
    tactic = z3::try_for(z3::repeat(tactic), 10000);
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