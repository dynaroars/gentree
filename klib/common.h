//
// Created by KH on 3/5/2020.
//

#ifndef IGEN4_COMMON_H
#define IGEN4_COMMON_H

#include <boost/container/container_fwd.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include "logging.h"

namespace z3 {
class expr;
}

namespace igen {

namespace bc = boost::container;

using str = std::string;
template<class T> using vec = std::vector<T>;
template<class T, class U> using map = bc::flat_map<T, U>;
template<class T> using set = bc::flat_set<T>;

template<class T, int Sz = 16> using sm_vec = bc::small_vector<T, Sz>;

template<class T>
using intrusive_ref_base_st = boost::intrusive_ref_counter<T, boost::thread_unsafe_counter>;
template<class T> using ptr = boost::intrusive_ptr<T>;

using expr = z3::expr;

}

#endif //IGEN4_COMMON_H
