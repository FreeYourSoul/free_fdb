cmake_minimum_required(VERSION 3.17)
project(free_fdb)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt REQUIRED)

add_library(free_fdb SHARED
        src/ffdb.cpp
        src/iterator.cpp
        include/free_fdb/ffdb.hh
        include/free_fdb/iterator.hh include/internal/future.cpp include/internal/future.hh test/ffdb_testcase.cpp test/iterator_testcase.cpp)

target_compile_features(free_fdb INTERFACE cxx_std_17)

target_include_directories(free_fdb PUBLIC include)