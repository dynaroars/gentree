//
// Created by KH on 3/5/2020.
//

#include "Config.h"
#include "Domain.h"

namespace igen {


Config::Config(PMutContext ctx) : Object(move(ctx)), values_(size_t(dom()->n_vars())) {

}


}