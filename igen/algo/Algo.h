//
// Created by kh on 3/14/20.
//

#ifndef IGEN4_ALGO_H
#define IGEN4_ALGO_H

#include <klib/common.h>

#include <boost/container/flat_map.hpp>
#include <boost/any.hpp>

namespace igen {

int run_interative_algorithm(const map<str, boost::any> &opts);

int run_interative_algorithm_2(const map<str, boost::any> &opts);

int run_analyzer(const map<str, boost::any> &opts);

}


#endif //IGEN4_ALGO_H
