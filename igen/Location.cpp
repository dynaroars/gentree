//
// Created by KH on 3/7/2020.
//

#include "Location.h"
#include "CoverageStore.h"

namespace igen {


Location::Location(PMutContext ctx, str name, int id) : Object(move(ctx)), name_(name), id_(id) {

}

void Location::add_cov_by_conf_id(int conf_id) {
    CHECK(cov_by_mut_ids().empty() || cov_by_mut_ids().back() < conf_id);
    cov_by_mut_ids().push_back(conf_id);
    cov_by_hash_.add(conf_id);
}


ptr<const Config> Location::const_config_iterator::dereference() const {
    return ctx->cov()->config(*base_reference());
}


}