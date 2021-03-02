cmake_minimum_required(VERSION 3.17)
project(free_fdb)

set(CMAKE_CXX_STANDARD 17)

add_library(free_fdb SHARED
        src/ffdb.cpp
        include/free_fdb/ffdb.hh)

target_include_directories(free_fdb PUBLIC include)