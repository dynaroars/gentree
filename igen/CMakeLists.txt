add_library(libigen4
        Context.cpp Context.h
        Config.cpp Config.h
        Domain.cpp Domain.h
        Z3Scope.h
        algo/IterAlgo.cpp
        c50/CTree.cpp c50/CTree.h
        runner/ProgramRunner.cpp runner/ProgramRunner.h
        CoverageStore.cpp CoverageStore.h
        Location.cpp Location.h
        c50/CNode.cpp c50/CNode.h
        runner/GCovRunner.cpp runner/GCovRunner.h
        builtin/programs.h builtin/programs.cpp builtin/fn_1.cpp builtin/impl.h builtin/fn_2.cpp algo/Analyzer.cpp algo/Algo.h algo/Iter2.cpp runner/ProgramRunnerMt.cpp runner/ProgramRunnerMt.h algo/DemoAlgo.cpp runner/OtterRunner.cpp runner/OtterRunner.h)

target_link_libraries(libigen4
        PRIVATE
        Boost::headers Boost::program_options Boost::timer Boost::filesystem Boost::thread Boost::iostreams
        rapidjson robinmap
        PUBLIC
        z3::libz3 klib)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(libigen4 PRIVATE RocksDB::rocksdb-shared)
else ()
    target_link_libraries(libigen4 PRIVATE RocksDB::rocksdb)
endif ()
