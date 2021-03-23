find_package(Catch2 QUIET)

add_executable(ffdb_test
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ffdb_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/iterator_testcase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/counter_testcase.cpp
        db_setup_test.hh)
target_link_libraries(ffdb_test free_fdb)
catch_discover_tests(ffdb_test)

