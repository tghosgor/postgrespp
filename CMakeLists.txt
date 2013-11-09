CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(postgrespp)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  MESSAGE(STATUS "Setting build type to 'Release' as none was specified.")
  SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # SET the possible values of build type for cmake-gui
endif()

SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release" "Debug")

SET(CMAKE_CXX_COMPILER "clang++")

SET(CMAKE_CXX_FLAGS "-std=c++11 -emit-llvm")
SET(CMAKE_CXX_FLAGS_DEBUG "-g3")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(CMAKE_SHARED_LINKER_FLAGS "-lpq -lboost_system -fPIC")
SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-emit-llvm")

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

ADD_SUBDIRECTORY(postgrespp)

FILE(COPY postgrespp/include DESTINATION build)
