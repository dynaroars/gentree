add_library(libigen4
        Context.cpp Context.h
        Config.cpp Config.h
        Domain.cpp Domain.h
        Z3Scope.h
        IterativeAlgorithm.cpp IterativeAlgorithm.h
        c50/CTree.cpp c50/CTree.h ProgramRunner.cpp ProgramRunner.h CoverageStore.cpp CoverageStore.h Location.cpp Location.h)

target_link_libraries(libigen4
        PRIVATE
        Boost::headers Boost::program_options Boost::timer Boost::filesystem
        PUBLIC
        z3::libz3 klib)
