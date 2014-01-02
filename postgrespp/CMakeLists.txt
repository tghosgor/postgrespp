INCLUDE_DIRECTORIES(include)

ADD_LIBRARY(postgrespp STATIC src/postgrespp.cpp)

target_link_libraries(postgrespp boost_system pq)

if(CMAKE_COMPILER_IS_GNUCXX)
	TARGET_LINK_LIBRARIES(postgrespp pthread)
endif()

INSTALL(FILES include/postgrespp.h DESTINATION include)
INSTALL(TARGETS postgrespp
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
