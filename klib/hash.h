//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_HASH_H
#define IGEN4_HASH_H


#include <boost/dynamic_bitset_fwd.hpp>
#include <vector>

namespace igen {

typedef std::pair<uint64_t, uint64_t> hash_t;

static constexpr hash_t hash128_empty = {0, 0};

hash_t calc_hash_128(const boost::dynamic_bitset<> &bs);

template<typename T>
hash_t calc_hash_128(const std::vector<T> &vec);

}


#endif //IGEN4_HASH_H
