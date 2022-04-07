cmake_minimum_required(VERSION 3.0)

project(test)
find_package(GTest REQUIRED)

file(GLOB src_files src/.*cpp)

set (tests
      tcp_test)

include_directories(include)
include_directories(${SOCEV_LIB_INCLUDE_DIR})

foreach(test ${tests})
  set (test_src_files "${src_files};${CMAKE_CURRENT_SOURCE_DIR}/src/${test}.cpp")

  add_executable(${test} ${test_src_files})
  add_test(NAME ${test} COMMAND ${test})
  target_link_libraries(${test} socev)
  target_link_libraries(${test} ${GTEST_LIBRARIES})
  target_link_libraries(${test} pthread)
endforeach()
