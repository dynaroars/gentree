//
// Created by KH on 3/5/2020.
//

#include "Context.h"
#include "Domain.h"

#include <fstream>

namespace igen {


void Context::set_option(const str &key, boost::any val) {
    options[key] = val;
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

    str stem = get_option_as<str>("filestem");
    str dom_path = stem + ".dom";
    std::ifstream ifs_dom(dom_path);
    CHECKF(ifs_dom, "Bad dom input file: {}", dom_path);
    ifs_dom >> (*dom());
}

void Context::cleanup() {
    dom_ = nullptr;
}


ptr<const Domain> Context::dom() const { return dom_; }

const ptr<Domain> &Context::dom() { return dom_; }

ptr<const Domain> Object::dom() const { return ctx_->dom(); }
}