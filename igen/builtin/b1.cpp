//
// Created by KH on 3/7/2020.
//

#include "impl.h"

FN(complex_1,
   VARS(s, t, u, v, x, y, z),
   DOMS(2, 2, 2, 2, 2, 2, 5), {
       if (((s || t) && (u || (!v && z == 2))) || (x && z >= 3) || (t && (u || (0 < z && z <= 2))))
           LOC("L1");
   })

FN(simple_1,
   VARS(a, b, c, d),
   DOMS(2, 2, 2, 2), {
       if ((a || b) && (c || d))
           LOC("L1");
   })