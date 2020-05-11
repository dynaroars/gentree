//
// Created by kh on 5/10/20.
//

#ifndef IGEN4_ANYUTILS_H
#define IGEN4_ANYUTILS_H

#include <string>

#include <boost/any.hpp>

namespace igen {

std::string any2string(const boost::any &anything);

long any2long(const boost::any &anything);

}

#endif //IGEN4_ANYUTILS_H
