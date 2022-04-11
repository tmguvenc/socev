cmake_minimum_required(VERSION 3.0)

project(socev)

enable_testing()
add_subdirectory(lib)
add_subdirectory(apps)
add_subdirectory(test)

add_custom_target(
  FormatSource
  COMMAND "${CMAKE_SOURCE_DIR}/format.sh")
  