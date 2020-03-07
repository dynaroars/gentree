add_library(libigen4
        Context.cpp Context.h
        Config.cpp Config.h
        Domain.cpp Domain.h
        Z3Scope.h
        AlgoDriver.cpp AlgoDriver.h
        c50/CTree.cpp c50/CTree.h)

target_link_libraries(libigen4
        PRIVATE
        Boost::headers Boost::program_options Boost::timer
        PUBLIC
        z3::libz3 klib)
