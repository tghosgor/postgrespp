find_package(GTest REQUIRED)

function(DECLARE_TEST NAME)
  add_executable(${NAME}_test ${NAME}_test.cpp)
  
  target_compile_options(${NAME}_test PUBLIC -pthread)
  target_link_options(${NAME}_test PUBLIC -pthread)
  target_link_libraries(${NAME}_test 
    ${GTEST_BOTH_LIBRARIES}
    postgrespp
    pqxx
  )
  
  add_test(test_${NAME} ${NAME}_test)
endfunction()

declare_test(connection)
declare_test(async_exec)
declare_test(async_exec_prepared)
