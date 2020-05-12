//
// Created by kh on 5/10/20.
//

#include "anyutils.h"

#include "logging.h"
#include "common.h"

namespace igen {

namespace {

bool is_char_ptr(const boost::any &operand) {
    try {
        boost::any_cast<char *>(operand);
        return true;
    }
    catch (const boost::bad_any_cast &) {
        return false;
    }
}

template<typename T>
bool try_cast_str(std::string &res, const boost::any &anything) {
    if (anything.type() == typeid(T)) {
        if constexpr (std::is_same_v<T, std::string>)
            res = boost::any_cast<T>(anything);
        else
            res = std::to_string(boost::any_cast<T>(anything));
        return true;
    } else {
        return false;
    }
}

}

std::string any2string(const boost::any &anything) {
    std::string res;
    if (try_cast_str<short>(res, anything)) return res;
    if (try_cast_str<int>(res, anything)) return res;
    if (try_cast_str<long>(res, anything)) return res;
    if (try_cast_str<double>(res, anything)) return res;
    if (try_cast_str<std::string>(res, anything)) return res;
    if (is_char_ptr(anything)) {
        return std::string(boost::any_cast<char *>(anything));
    }
    CHECK(0);
    return "";
}

template<typename From, typename To>
bool try_cast_to(To &res, const boost::any &anything) {
    if (anything.type() == typeid(From)) {
        res = (To) boost::any_cast<From>(anything);
        return true;
    } else {
        return false;
    }
}

template<typename To>
To any2num(const boost::any &anything) {
    To res;
    if (try_cast_to<short, To>(res, anything)) return res;
    if (try_cast_to<int, To>(res, anything)) return res;
    if (try_cast_to<long, To>(res, anything)) return res;
    if (try_cast_to<float, To>(res, anything)) return res;
    if (try_cast_to<double, To>(res, anything)) return res;
    return kMIN<To>;
}

long any2long(const boost::any &anything) {
    return any2num<long>(anything);
}

double any2double(const boost::any &anything) {
    return any2num<double>(anything);
}

}