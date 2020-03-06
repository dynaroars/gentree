//
// Created by KH on 3/5/2020.
//

#include "Context.h"

namespace igen {


void Context::set_option(const str &key, boost::any val) {
    options[key] = val;
}

boost::any Context::get_option(const str &key) const {
    auto it = options.find(key);
    CHECK(it != options.end());
    return it->second;
}

Context::Context() : z3solver(z3ctx), z3true(z3ctx.bool_val(true)), z3false(z3ctx.bool_val(false)) {
}

}