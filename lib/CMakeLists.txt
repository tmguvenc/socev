cmake_minimum_required(VERSION 3.0)

project(socev)

file(GLOB SRC_FILES "src/*.c")
file(GLOB HDR_FILES "include/*.h")

include_directories(include)

set(SOCEV_LIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE STRING "")

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})