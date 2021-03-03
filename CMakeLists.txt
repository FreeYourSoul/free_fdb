cmake_minimum_required(VERSION 3.17)
project(free_fdb)

set(CMAKE_CXX_STANDARD 17)

add_library(free_fdb SHARED
        src/ffdb.cpp
        src/iterator.cpp
        include/free_fdb/ffdb.hh
        include/free_fdb/iterator.hh include/internal/future.cpp include/internal/future.hh)

target_include_directories(free_fdb PUBLIC include)