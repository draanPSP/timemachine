project(utils VERSION 1.0)

add_library(utils INTERFACE)

target_include_directories(utils INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
