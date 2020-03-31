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

# ZSTD \/ \/ \/ \/ \/ \/ \/ \/
find_path(zstd_INCLUDE_DIRS
        NAMES zstd.h
        HINTS ${zstd_ROOT_DIR}/include)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ZSTD_PREFER_LIBRARY libzstd.so)
else ()
    set(ZSTD_PREFER_LIBRARY libzstd.a)
endif ()
find_library(zstd_LIBRARIES
        NAMES ${ZSTD_PREFER_LIBRARY} zstd
        HINTS ${zstd_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zstd DEFAULT_MSG zstd_LIBRARIES zstd_INCLUDE_DIRS)

mark_as_advanced(
        zstd_LIBRARIES
        zstd_INCLUDE_DIRS)

if (NOT zstd_FOUND)
    message(FATAL "Cannot find zstd")
endif ()
add_library(zstd::zstd UNKNOWN IMPORTED)
set_target_properties(zstd::zstd
        PROPERTIES
        IMPORTED_LOCATION ${zstd_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${zstd_INCLUDE_DIRS})
# ZSTD /\ /\ /\ /\ /\ /\ /\ /\

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
find_package(Boost REQUIRED COMPONENTS headers program_options timer filesystem)

# ===

add_subdirectory(third_party/rapidjson)
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
else ()
    target_compile_options(igen4 PUBLIC -flto)
endif ()

add_subdirectory(kutil)