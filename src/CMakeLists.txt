add_library(postgrespp basic_connection.cpp)

target_include_directories(postgrespp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(postgrespp PUBLIC -pthread -std=c++17)
target_link_options(postgrespp PUBLIC -pthread)
target_link_libraries(postgrespp pq)
file(GLOB POSTGRESPP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../include/**.hpp)
set_target_properties(postgrespp PROPERTIES
  PUBLIC_HEADER "${POSTGRESPP_HEADERS}")
install(TARGETS postgrespp
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION include/postgrespp)
