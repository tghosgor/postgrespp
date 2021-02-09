find_package(GTest REQUIRED)

add_executable(connection_test connection_test.cpp)

target_compile_options(connection_test PUBLIC -pthread)
target_link_options(connection_test PUBLIC -pthread)
target_link_libraries(connection_test 
  ${GTEST_BOTH_LIBRARIES}
  postgrespp
  pqxx
)

add_test(test_connection connection_test)
