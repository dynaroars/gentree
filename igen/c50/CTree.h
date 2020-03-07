//
// Created by KH on 3/6/2020.
//

#ifndef IGEN4_CTREE_H
#define IGEN4_CTREE_H

#include <klib/common.h>
#include <igen/Context.h>

namespace igen {

class CTree : public Object {
public:
    explicit CTree(PMutContext ctx);

private:
};

using PCTree = ptr<const CTree>;
using PMutCTree = ptr<CTree>;

}

#endif //IGEN4_CTREE_H
