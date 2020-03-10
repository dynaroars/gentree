add_library(klib
        common.h
        logging.h
        hash.cpp hash.h print_stl.h custom_container.h math.h random.cpp
        MurmurHash3.h MurmurHash3.cpp)

target_link_libraries(klib
        PRIVATE
        Boost::headers
        PUBLIC
        fmt::fmt glog::glog)

target_include_directories(klib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)