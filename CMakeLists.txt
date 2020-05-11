cmake_minimum_required(VERSION 3.15)
project(igen4)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wconversion -pedantic)
endif ()

# === Third party ======================================
find_package(fmt REQUIRED CONFIG)
message(STATUS "Found fmt ${fmt_FIND_VERSION}, dir: ${fmt_DIR}")

find_package(glog REQUIRED CONFIG)
message(STATUS "Found glog ${glog_FIND_VERSION}, dir: ${glog_DIR}")

find_package(Z3 REQUIRED CONFIG)
message(STATUS "Found Z3 ${Z3_VERSION_STRING}, Z3_DIR: ${Z3_DIR}")

include(third_party/cmake/find_zstd.cmake)

find_package(ZLIB REQUIRED)
message(STATUS "Found ZLIB ${ZLIB_VERSION_STRING}, ZLIB_DIR: ${ZLIB_INCLUDE_DIRS}")

find_package(RocksDB REQUIRED CONFIG)
message(STATUS "Found RocksDB ${RocksDB_VERSION_STRING}, RocksDB_DIR: ${RocksDB_DIR}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    IF (WIN32)
        set(Boost_COMPILER mgw92)
    ENDIF ()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(Boost_USE_STATIC_LIBS OFF)
    else ()
        set(Boost_USE_STATIC_LIBS ON)
    endif ()
endif ()
find_package(Boost REQUIRED COMPONENTS headers program_options timer filesystem thread iostreams regex)

# ===

add_subdirectory(third_party/rapidjson)
add_subdirectory(third_party/pugixml)
add_subdirectory(third_party/robinmap)
add_subdirectory(klib)
add_subdirectory(igen)

# === Main code ======================================

add_executable(igen4
        main.cpp)

target_link_libraries(igen4 PRIVATE
        Boost::headers Boost::program_options Boost::timer
        z3::libz3 klib libigen4)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
elseif (IGEN_LTO)
    target_compile_options(igen4 PUBLIC -flto)
    target_compile_options(libigen4 PUBLIC -flto)
    target_compile_options(klib PUBLIC -flto)
endif ()

add_subdirectory(kutil)