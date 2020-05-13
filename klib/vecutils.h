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

template<typename T, typename Out = double>
Out vec_median(const std::vector<T> &vec) {
    auto size = vec.size();
    if (size == 0)
        throw std::domain_error("median of an empty vector");
    auto mid = size / 2;
    return size % 2 == 0 ? Out(vec[mid] + vec[mid - 1]) / 2 : vec[mid];
}

template<typename T, typename Out = double>
Out vec_median(const std::vector<T> &vec, size_t beg, size_t end) {
    if (beg > end)
        throw std::domain_error("beg > end");
    if (end > vec.size())
        throw std::domain_error("end > vec.size()");
    auto size = end - beg;
    if (size == 0)
        throw std::domain_error("median of an empty vector");
    auto mid = size / 2;
    return size % 2 == 0 ? Out(vec[beg + mid] + vec[beg + mid - 1]) / 2 : vec[beg + mid];
}

template<typename T, typename Out = double>
Out vec_mean(const std::vector<T> &vec) {
    Out sum = 0;
    for (const auto &v : vec) sum += v;
    return sum / (Out) vec.size();
}

// Semi-Interquartile Range
template<typename T, typename Out = double>
Out vec_sir(const std::vector<T> &vec) {
    auto size = vec.size();
    if (size == 0)
        throw std::domain_error("sir of an empty vector");
    auto mid = size / 2;
    Out Q1 = vec_median<T, Out>(vec, 0, std::max<decltype(mid)>(mid, 1));
    Out Q3;
    if (size % 2 == 0) {
        Q3 = vec_median<T, Out>(vec, std::min(mid, size - 1), size);
    } else {
        Q3 = vec_median<T, Out>(vec, std::min(mid + 1, size - 1), size);
    }
    return (Q3 - Q1) / 2;
}

}

#endif //IGEN4_VECUTILS_H
