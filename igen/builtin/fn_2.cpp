//
// Created by KH on 3/12/2020.
//

#include "impl.h"

FN_MODULE(fn_2)


FN(ex4,
   VARS(a, b, c, d, e),
   DOMS(2, 2, 2, 2, 2), {
       LOC("LS");
       if (a == 1) {
           LOC("a1");
           if (b == 1) {
               LOC("a1b1");
               if (c == 1) {
                   LOC("a1b1c1");
                   if (d == 1) {
                       LOC("a1b1c1d1");
                       if (e == 1) {
                           LOC("a1b1c1d1e1");
                       } else {
                           LOC("a1b1c1d1e0");
                       }
                   } else {
                       LOC("a1b1c1d0");
                       if (e == 1) {
                           LOC("a1b1c1d0e1");
                       } else {
                           LOC("a1b1c1d0e0");
                       }
                   }
               } else {
                   LOC("a1b1c0");
                   if (d == 1) {
                       LOC("a1b1c0d1");
                       if (e == 1) {
                           LOC("a1b1c0d1e1");
                       } else {
                           LOC("a1b1c0d1e0");
                       }
                   } else {
                       LOC("a1b1c0d0");
                       if (e == 1) {
                           LOC("a1b1c0d0e1");
                       } else {
                           LOC("a1b1c0d0e0");
                       }
                   }

               }
           } else {
               LOC("a1b0");
               if (c == 1) {
                   LOC("a1b0c1");
                   if (d == 1) {
                       LOC("a1b0c1d1");
                       if (e == 1) {
                           LOC("a1b0c1d1e1");
                       } else {
                           LOC("a1b0c1d1e0");
                       }
                   } else {
                       LOC("a1b0c1d0");
                       if (e == 1) {
                           LOC("a1b0c1d0e1");
                       } else {
                           LOC("a1b0c1d0e0");
                       }
                   }

               } else {
                   LOC("a1b0c0");
                   if (d == 1) {
                       LOC("a1b0c0d1");
                       if (e == 1) {
                           LOC("a1b0c0d1e1");
                       } else {
                           LOC("a1b0c0d1e0");
                       }
                   } else {
                       LOC("a1b0c0d0");
                       if (e == 1) {
                           LOC("a1b0c0d0e1");
                       } else {
                           LOC("a1b0c0d0e0");
                       }
                   }

               }
           }
       } else {
           LOC("a0");
           if (b == 1) {
               LOC("a0b1");
               if (c == 1) {
                   LOC("a0b1c1");
                   if (d == 1) {
                       LOC("a0b1c1d1");
                       if (e == 1) {
                           LOC("a0b1c1d1e1");
                       } else {
                           LOC("a0b1c1d1e0");
                       }
                   } else {
                       LOC("a0b1c1d0");
                       if (e == 1) {
                           LOC("a0b1c1d0e1");
                       } else {
                           LOC("a0b1c1d0e0");
                       }
                   }

               } else {
                   LOC("a0b1c0");
                   if (d == 1) {
                       LOC("a0b1c0d1");
                       if (e == 1) {
                           LOC("a0b1c0d1e1");
                       } else {
                           LOC("a0b1c0d1e0");
                       }
                   } else {
                       LOC("a0b1c0d0");
                       if (e == 1) {
                           LOC("a0b1c0d0e1");
                       } else {
                           LOC("a0b1c0d0e0");
                       }
                   }

               }
           } else {
               LOC("a0b0");
               if (c == 1) {
                   LOC("a0b0c1");
                   if (d == 1) {
                       LOC("a0b0c1d1");
                       if (e == 1) {
                           LOC("a0b0c1d1e1");
                       } else {
                           LOC("a0b0c1d1e0");
                       }
                   } else {
                       LOC("a0b0c1d0");
                       if (e == 1) {
                           LOC("a0b0c1d0e1");
                       } else {
                           LOC("a0b0c1d0e0");
                       }
                   }

               } else {
                   LOC("a0b0c0");
                   if (d == 1) {
                       LOC("a0b0c0d1");
                       if (e == 1) {
                           LOC("a0b0c0d1e1");
                       } else {
                           LOC("a0b0c0d1e0");
                       }
                   } else {
                       LOC("a0b0c0d0");
                       if (e == 1) {
                           LOC("a0b0c0d0e1");
                       } else {
                           LOC("a0b0c0d0e0");
                       }
                   }

               }
           }
       }
   })


FN(ex4_2,
   VARS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, t),
   DOMS(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2), {
       LOC("LS");
       if (a == 1) {
           LOC("a1");
           if (b == 1) {
               LOC("a1b1");
               if (c == 1) {
                   LOC("a1b1c1");
                   if (d == 1) {
                       LOC("a1b1c1d1");
                       if (e == 1) {
                           LOC("a1b1c1d1e1");
                       } else {
                           LOC("a1b1c1d1e0");
                       }
                   } else {
                       LOC("a1b1c1d0");
                       if (e == 1) {
                           LOC("a1b1c1d0e1");
                       } else {
                           LOC("a1b1c1d0e0");
                       }
                   }
               } else {
                   LOC("a1b1c0");
                   if (d == 1) {
                       LOC("a1b1c0d1");
                       if (e == 1) {
                           LOC("a1b1c0d1e1");
                       } else {
                           LOC("a1b1c0d1e0");
                       }
                   } else {
                       LOC("a1b1c0d0");
                       if (e == 1) {
                           LOC("a1b1c0d0e1");
                       } else {
                           LOC("a1b1c0d0e0");
                       }
                   }

               }
           } else {
               LOC("a1b0");
               if (c == 1) {
                   LOC("a1b0c1");
                   if (d == 1) {
                       LOC("a1b0c1d1");
                       if (e == 1) {
                           LOC("a1b0c1d1e1");
                       } else {
                           LOC("a1b0c1d1e0");
                       }
                   } else {
                       LOC("a1b0c1d0");
                       if (e == 1) {
                           LOC("a1b0c1d0e1");
                       } else {
                           LOC("a1b0c1d0e0");
                       }
                   }

               } else {
                   LOC("a1b0c0");
                   if (d == 1) {
                       LOC("a1b0c0d1");
                       if (e == 1) {
                           LOC("a1b0c0d1e1");
                       } else {
                           LOC("a1b0c0d1e0");
                       }
                   } else {
                       LOC("a1b0c0d0");
                       if (e == 1) {
                           LOC("a1b0c0d0e1");
                       } else {
                           LOC("a1b0c0d0e0");
                       }
                   }

               }
           }
       } else {
           LOC("a0");
           if (b == 1) {
               LOC("a0b1");
               if (c == 1) {
                   LOC("a0b1c1");
                   if (d == 1) {
                       LOC("a0b1c1d1");
                       if (e == 1) {
                           LOC("a0b1c1d1e1");
                       } else {
                           LOC("a0b1c1d1e0");
                       }
                   } else {
                       LOC("a0b1c1d0");
                       if (e == 1) {
                           LOC("a0b1c1d0e1");
                       } else {
                           LOC("a0b1c1d0e0");
                       }
                   }

               } else {
                   LOC("a0b1c0");
                   if (d == 1) {
                       LOC("a0b1c0d1");
                       if (e == 1) {
                           LOC("a0b1c0d1e1");
                       } else {
                           LOC("a0b1c0d1e0");
                       }
                   } else {
                       LOC("a0b1c0d0");
                       if (e == 1) {
                           LOC("a0b1c0d0e1");
                       } else {
                           LOC("a0b1c0d0e0");
                       }
                   }

               }
           } else {
               LOC("a0b0");
               if (c == 1) {
                   LOC("a0b0c1");
                   if (d == 1) {
                       LOC("a0b0c1d1");
                       if (e == 1) {
                           LOC("a0b0c1d1e1");
                       } else {
                           LOC("a0b0c1d1e0");
                       }
                   } else {
                       LOC("a0b0c1d0");
                       if (e == 1) {
                           LOC("a0b0c1d0e1");
                       } else {
                           LOC("a0b0c1d0e0");
                       }
                   }

               } else {
                   LOC("a0b0c0");
                   if (d == 1) {
                       LOC("a0b0c0d1");
                       if (e == 1) {
                           LOC("a0b0c0d1e1");
                       } else {
                           LOC("a0b0c0d1e0");
                       }
                   } else {
                       LOC("a0b0c0d0");
                       if (e == 1) {
                           LOC("a0b0c0d0e1");
                       } else {
                           LOC("a0b0c0d0e0");
                       }
                   }

               }
           }
       }
   })