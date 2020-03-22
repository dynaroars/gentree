//
// Created by KH on 3/10/2020.
//

#ifndef IGEN4_VECUTILS_H
#define IGEN4_VECUTILS_H

#include <vector>

namespace igen {

template<class T>
void vec_move_append(std::vector<T> &dst, std::vector<T> &src) {
    if (dst.empty()) {
        dst = std::move(src);
    } else {
        dst.reserve(dst.size() + src.size());
        std::move(std::begin(src), std::end(src), std::back_inserter(dst));
        src.clear();
    }
}

template<class T>
void vec_append(std::vector<T> &dst, const std::vector<T> &src) {
    if (dst.empty()) {
        dst = src;
    } else {
        dst.reserve(dst.size() + src.size());
        dst.insert(std::end(dst), std::begin(src), std::end(src));
    }
}

template<typename Vector>
void unordered_erase(Vector &v, typename Vector::iterator it) {
    *it = std::move(v.back());
    v.pop_back();
}

}

#endif //IGEN4_VECUTILS_H
