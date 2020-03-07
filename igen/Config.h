//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_CONFIG_H
#define IGEN4_CONFIG_H

#include "Context.h"

namespace igen {

class Config : public Object {

};

using PConfig = ptr<const Config>;
using PMutConfig = ptr<const Config>;

}

#endif //IGEN4_CONFIG_H
