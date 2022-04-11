cmake_minimum_required(VERSION 3.0)

project(test)
find_package(GTest REQUIRED)

set (tests
      tcp_test)

include_directories(include)
include_directories(${SOCEV_LIB_INCLUDE_DIR})

foreach(test ${tests})
  add_executable(${test} ${CMAKE_CURRENT_SOURCE_DIR}/src/${test}.cpp)
  add_test(NAME ${test} COMMAND ${test})
  target_link_libraries(${test} socev)
  target_link_libraries(${test} ${GTEST_LIBRARIES})
  target_link_libraries(${test} pthread)
  add_dependencies(${test} FormatSource)
endforeach()
