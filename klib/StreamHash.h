//
// Created by KH on 3/10/2020.
//

#ifndef IGEN4_STREAMHASH_H
#define IGEN4_STREAMHASH_H

#include <utility>

#include "hash.h"

namespace igen {

class StreamHash {
public:
    StreamHash(hash_t IV, uint32_t counter) : KEY(std::move(IV)), counter(counter) {}

    StreamHash() : StreamHash({0xdbafd606665ff759ULL, 0x6d2636fbaebfa7caULL}, 0x4483d451) {}

    void add(int val) {
        for (int i = 0; i < ROUNDS; i++) {
            R(KEY.first, KEY.second, (uint64_t(counter) << 32u) ^ uint32_t(val));
            R(VAL.first, VAL.second, KEY.second);
            counter++;
        }
    }

    void add(const hash_t &h) {
        VAL.first ^= h.first, VAL.second |= h.second;
        for (int i = 0; i < ROUNDS; i++) {
            R(KEY.first, KEY.second, uint64_t(counter));
            R(VAL.first, VAL.second, KEY.second);
            counter++;
        }
    }

    [[nodiscard]] hash_t digest() const {
        return VAL;
//        hash_t ret = VAL, tkey = KEY;
//        uint64_t X = (uint64_t(counter) << 32u) | counter;
//        X = ROR(X, 16);
//        for (int i = 0; i < FINAL_ROUNDS; i++) {
//            R(tkey.first, tkey.second, X ^ uint64_t(i));
//            R(ret.first, ret.second, tkey.second);
//        }
//        return ret;
    }

private:
    static constexpr int ROUNDS = 2;
//    static constexpr int FINAL_ROUNDS = 2;

    static inline uint64_t ROR(const uint64_t x, const uint64_t r) {
        return (x >> r) | (x << (64 - r));
    }

    static inline uint64_t ROL(const uint64_t x, const uint64_t r) {
        return (x << r) | (x >> (64 - r));
    }

    static inline void R(uint64_t &x, uint64_t &y, const uint64_t k) {
        x = ROR(x, 8), x += y, x ^= k, y = ROL(y, 3), y ^= x;
    }

private:
    hash_t KEY;
    hash_t VAL;
    uint32_t counter;
};

}

#endif //IGEN4_STREAMHASH_H
