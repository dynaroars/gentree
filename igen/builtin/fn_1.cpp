//
// Created by KH on 3/7/2020.
//

#include "impl.h"

FN_MODULE(fn_1)

FN(c1,
   VARS(s, t, u, v, x, y, z),
   DOMS(2, 2, 2, 2, 2, 2, 5), {
       if (((s || t) && (u || (!v && z == 2))) || (x && z >= 3) || (t && (u || (0 < z && z <= 2))))
           LOC("L1");
   })

FN(s1,
   VARS(a, b, c, d, e),
   DOMS(2, 2, 2, 2, 2), {
       if ((a || b) && (c || d))
           LOC("L1");
   })

FN(s2,
   VARS(a, b, c),
   DOMS(2, 2, 2), {
       if (a && b && c)
           LOC("L1");
   })

FN(s3,
   VARS(a, b, c),
   DOMS(2, 2, 2), {
       if ((a && b) || c)
           LOC("L1");
   })