add_executable(kutil
        kutil.cpp db_utils.cpp kutil.h)

target_link_libraries(kutil PRIVATE
        Boost::headers Boost::program_options Boost::timer
        z3::libz3 klib libigen4)