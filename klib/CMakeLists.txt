add_library(klib
        common.h
        logging.h
        HashUtil.cpp HashUtil.h print_stl.h custom_container.h math.h)

target_link_libraries(klib
        PRIVATE
        Boost::headers
        PUBLIC
        fmt::fmt glog::glog)

target_include_directories(klib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)