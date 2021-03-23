cmake_minimum_required(VERSION 3.16)
project(free_fdb)

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

find_package(fmt REQUIRED)
find_package(Catch2 REQUIRED)

include(Catch)

add_library(free_fdb STATIC
        src/ffdb.cpp
        src/iterator.cpp
        include/free_fdb/ffdb.hh
        include/free_fdb/iterator.hh
        include/internal/future.hh)

target_link_libraries(free_fdb PUBLIC fdb_c)
target_link_libraries(free_fdb PRIVATE pthread fmt::fmt)
target_compile_features(free_fdb INTERFACE cxx_std_17)
target_include_directories(free_fdb PRIVATE include)


set(DEF_INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/free_fdb)
install(DIRECTORY include/free_fdb DESTINATION include/)
install(TARGETS free_fdb EXPORT free_fdbConfig)
install(EXPORT free_fdbConfig DESTINATION ${DEF_INSTALL_CMAKE_DIR})

enable_testing()
add_subdirectory(test)
