//
// Created by KH on 3/7/2020.
//

#include "CoverageStore.h"

namespace igen {


CoverageStore::CoverageStore(igen::PMutContext ctx) : Object(move(ctx)) {
}

void intrusive_ptr_release(CoverageStore *d) { intrusive_ptr_add_ref(d); }


}