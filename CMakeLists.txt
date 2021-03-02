cmake_minimum_required(VERSION 3.19)

project(postgrespp CXX)

add_subdirectory(src)

if(POSTGRESPP_BUILD_TESTS)
  add_subdirectory(test)
endif()
