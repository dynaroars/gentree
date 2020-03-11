//
// Created by KH on 3/5/2020.
//

#include "hash.h"

#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include <boost/dynamic_bitset.hpp>

#include "MurmurHash3.h"


namespace igen {


static const uint32_t HASH_SEED = 0xCDAB23EF;

hash128_t calc_hash_128(const boost::dynamic_bitset<> &bs) {
    using block_type = boost::dynamic_bitset<>::block_type;
    const std::vector<block_type> &blocks = bs.m_bits; // dynamic_bit maintains zero unused bit by m_zero_unused_bits. Hopefully!

    uint64_t res[2];
    MurmurHash3_x64_128(blocks.data(), int(blocks.size() * sizeof(block_type)), HASH_SEED, &res);
    return {res[0], res[1]};
}

template<typename T>
hash128_t calc_hash_128(const std::vector<T> &vec) {
    uint64_t res[2];
    MurmurHash3_x64_128(vec.data(), int(vec.size() * sizeof(T)), HASH_SEED, &res);
    return {res[0], res[1]};
}

template
hash128_t calc_hash_128<int>(const std::vector<int> &vec);

}