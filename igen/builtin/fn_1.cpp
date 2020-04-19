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
   VARS(s, t, u, v, x, y, z, a, b, c, e, f, g, h),
   DOMS(2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2), {
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


       /////
       if ((s || t) && (y || z)) {
           LOC("L6");
       }

       if (x && (s || t) && (y || z)) {
           LOC("L7");
           LOC("L7a");
       }

       if ((u || v)) {
           LOC("L8");
           if (x || y) {
               LOC("L9");
           }
           if (x && y) {
               LOC("L10");
               LOC("L10");
           }
       }
   })

FN(ex_simple,
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
   })


FN(ex_2,
   VARS(s, t, u, v, x, y, z, a, b, c, d, e, f, g, h),
   DOMS(2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2), {
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


       /////
       if ((s || t) && (y || z)) {
           LOC("L6");
       }

       if (x && (s || t) && (y || z)) {
           LOC("L7");
           LOC("L7a");
       }

       if ((u || v)) {
           LOC("L8");
           if (x || y) {
               LOC("L9");
           }
           if (x && y) {
               LOC("L10");
               LOC("L10");
           }
       }
   })


FN(ex_paper,
   VARS(x, s, t, u, v, a, b, c, d),
   DOMS(3, 2, 2, 2, 2, 3, 3, 3, 3), {
       LOC("L0");
       if (a == 1 || b == 2) {
           LOC("L1");
       } else if (c == 0 && d == 1) {
           LOC("L2");
       }
       if (u && v) {
           LOC("L3"); // u=1 & v=1
           LOC("L3-1");
           RETURN;
       } else {
           LOC("L4"); // u=0 | v=0
           if (s && x == 2) {
               LOC("L5"); // s=1 & x=2 & (u=0 | v=0)
               RETURN;
           }
       }
       LOC("L6"); // a_1 := x=0 | x=1 | s=0;  (u=0 & a_1) | (v=0 & a_1)
       if (x == 2) {
           LOC("L7"); // x=2 & s=0 & (u=0 | v=0)
           if (u || v) {
               LOC("L8"); // x=2 & s=0 & ((u=1 & v=0) | (u=0 & v=1))
               LOC("L8-1");
           }
       }
   })


FN(ex_paper_l,
   VARS(x, s, t, u, v, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p),
   DOMS(3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2), {
       if (u && v) {
           LOC("L0"); // u=1 & v=1
           LOC("L0-1");
           RETURN;
       } else {
           LOC("L1"); // u=0 | v=0
           if (s && x == 2) {
               LOC("L2"); // s=1 & x=2 & (u=0 | v=0)
               RETURN;
           }
       }
       LOC("L3"); // a_1 := x=0 | x=1 | s=0;  (u=0 & a_1) | (v=0 & a_1)
       if (x == 2) {
           LOC("L4"); // x=2 & s=0 & (u=0 | v=0)
           if (u || v) {
               LOC("L5"); // x=2 & s=0 & ((u=1 & v=0) | (u=0 & v=1))
               LOC("L5-1");
           } else {
               LOC("L6"); // x=2 & s=0 & u=0 & v=0
           }
       }
   })