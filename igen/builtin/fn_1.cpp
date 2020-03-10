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


FN(c2,
   VARS(s, t, u, v, x, y, z, a, b, c),
   DOMS(2, 2, 2, 2, 2, 2, 5, 2, 2, 2), {
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

FN(s4,
   VARS(a, b, c),
   DOMS(2, 2, 2), {
       if (a || b || c)
           LOC("L1");
   })


FN(ex,
   VARS(s, t, u, v, x, y, z),
   DOMS(2, 2, 2, 2, 2, 2, 5), {
       int max_z = 3;
       if (x && y) {
           LOC("L0"); //x & y
           if (!(0 < z && z < max_z)) {
               LOC("L1"); //x & y & (z=0|3|4)
           }
       } else {
           LOC("L2"); // !x|!y
           LOC("L2a"); // !x|!y
       }

       LOC("L3"); // true
       if (u && v) {
           LOC("L4"); //u&v
           if (s || t) {
               LOC("L5");  // (s|t) & (u&v)
           }
       }
       if ((s || t) && (u || v))
           LOC("L5");
   })