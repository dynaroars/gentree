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
}

template<typename T>
bool try_cast_long(long &res, const boost::any &anything) {
    if (anything.type() == typeid(T)) {
        res = (long) boost::any_cast<T>(anything);
        return true;
    } else {
        return false;
    }
}

long any2long(const boost::any &anything) {
    long res;
    if (try_cast_long<short>(res, anything)) return res;
    if (try_cast_long<int>(res, anything)) return res;
    if (try_cast_long<long>(res, anything)) return res;
    return kMIN<long>;
}

}