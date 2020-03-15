//
// Created by kh on 3/14/20.
//

#ifndef IGEN4_ALGO_H
#define IGEN4_ALGO_H

#include <boost/program_options/variables_map.hpp>

namespace igen {

int run_interative_algorithm(const boost::program_options::variables_map &vm);

int run_analyzer(const boost::program_options::variables_map &vm);

}


#endif //IGEN4_ALGO_H
