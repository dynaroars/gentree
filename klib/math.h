//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_MATH_H
#define IGEN4_MATH_H

#include <cmath>

namespace igen {

inline double log2(double v) {
    return v <= 0 ? 0 : std::log2(v);
}

}

#endif //IGEN4_MATH_H
