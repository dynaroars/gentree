add_library(libigen4
        Context.cpp Context.h
        Config.cpp Config.h
        Domain.cpp Domain.h
        Z3Scope.h
        algo/IterAlgo.cpp algo/IterAlgo.h
        c50/CTree.cpp c50/CTree.h
        ProgramRunner.cpp ProgramRunner.h
        CoverageStore.cpp CoverageStore.h
        Location.cpp Location.h
        c50/CNode.cpp c50/CNode.h
        GCovRunner.cpp GCovRunner.h
        builtin/programs.h builtin/programs.cpp builtin/fn_1.cpp builtin/impl.h builtin/fn_2.cpp)

target_link_libraries(libigen4
        PRIVATE
        Boost::headers Boost::program_options Boost::timer Boost::filesystem
        rapidjson
        PUBLIC
        z3::libz3 klib)
