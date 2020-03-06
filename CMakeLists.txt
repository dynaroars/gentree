cmake_minimum_required(VERSION 3.15)
project(igen4)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wconversion -pedantic)
endif ()

# === Config ======================================

add_subdirectory(klib)
add_subdirectory(third_party/smhash)

# ===

add_executable(igen4
        main.cpp)

target_include_directories(igen4 PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/third_party)

target_link_libraries(igen4 PRIVATE klib smhash)

# === Third party ======================================
find_package(fmt REQUIRED CONFIG)
message(STATUS "Found fmt ${fmt_FIND_VERSION}, dir: ${fmt_DIR}")
target_link_libraries(igen4 PRIVATE fmt::fmt)

find_package(spdlog REQUIRED CONFIG)
message(STATUS "Found spdlog ${spdlog_FIND_VERSION}, dir: ${spdlog_DIR}")
target_link_libraries(igen4 PRIVATE spdlog::spdlog)

find_package(Z3 REQUIRED CONFIG)
message(STATUS "Found Z3 ${Z3_VERSION_STRING}, Z3_DIR: ${Z3_DIR}")
target_link_libraries(igen4 PRIVATE ${Z3_LIBRARIES})

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
find_package(Boost REQUIRED COMPONENTS headers program_options filesystem timer)
target_link_libraries(igen4 PRIVATE
        Boost::headers Boost::program_options Boost::filesystem Boost::timer)

