cmake_minimum_required(VERSION 3.0)

project(apps)

file(GLOB SRC_FILES "src/*.c")
file(GLOB HDR_FILES "include/*.h")

include_directories(include)
include_directories(${SOCEV_LIB_INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_dependencies(${PROJECT_NAME} socev)
target_link_libraries(${PROJECT_NAME} socev)

add_dependencies(${PROJECT_NAME} FormatSource)
