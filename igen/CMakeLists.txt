add_library(igen4_lib
        Context.cpp Context.h Config.cpp Config.h Domain.cpp Domain.h Z3Scope.h)

target_link_libraries(igen4_lib
        PRIVATE
        Boost::headers Boost::program_options Boost::timer
        PUBLIC
        z3::libz3 klib)
