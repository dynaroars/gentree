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

template<typename K, typename V>
std::vector<K> get_keys_as_vec(const map <K, V> &m) {
    std::vector<K> res;
    res.reserve(m.size());
    for (const auto &kv : m) res.push_back(kv.first);
    return res;
}

template<typename T>
long vec_median(const std::vector<T> &vec) {
    auto size = vec.size();
    if (size == 0)
        throw std::domain_error("median of an empty vector");
    auto mid = size / 2;
    return size % 2 == 0 ? (vec[mid] + vec[mid - 1]) / 2 : vec[mid];
}

}

#endif //IGEN4_VECUTILS_H
