//
// Created by KH on 3/7/2020.
//

#include "Location.h"
#include "CoverageStore.h"

namespace igen {


Location::Location(PMutContext ctx, str name, int id) : Object(move(ctx)), name_(name), id_(id) {

}


const ptr<const Config> Location::const_config_iterator::dereference() const {
    return ctx->cov()->config(*base_reference());
}


}