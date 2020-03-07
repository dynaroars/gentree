//
// Created by KH on 3/5/2020.
//

#include "Context.h"
#include "Domain.h"
#include "ProgramRunner.h"
#include "CoverageStore.h"

#include <fstream>

namespace igen {


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

Context::Context() :
        z3solver(z3ctx), z3true(z3ctx.bool_val(true)), z3false(z3ctx.bool_val(false)) {
}

void Context::init() {
    dom_ = new Domain(this);
    program_runner_ = new ProgramRunner(this);
    coverage_store_ = new CoverageStore(this);
}

void Context::cleanup() {
    coverage_store_ = nullptr;
    program_runner_ = nullptr;
    dom_ = nullptr;
}


ptr<const Domain> Context::dom() const { return dom_; }

const ptr<Domain> &Context::dom() { return dom_; }

ptr<const Domain> Object::dom() const { return ctx_->dom(); }

ptr<const CoverageStore> Object::cov() const { return ctx_->cov(); }

ptr<const ProgramRunner> Context::program_runner() const { return program_runner_; }

const ptr<ProgramRunner> &Context::program_runner() { return program_runner_; }

ptr<const CoverageStore> Context::cov() const { return coverage_store_; }

const ptr<CoverageStore> &Context::cov() { return coverage_store_; }
}