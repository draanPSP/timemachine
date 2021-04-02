cmake_minimum_required(VERSION 3.16)

project(timemachine VERSION 1.0 LANGUAGES C CXX ASM)

set (CMAKE_CXX_STANDARD 20)

add_compile_options(-Os -fno-exceptions $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

add_subdirectory(iplsdk)
add_subdirectory(tools)

add_subdirectory(tm_common)
add_subdirectory(tm_msipl)
add_subdirectory(tm_loader)
add_subdirectory(tm_firmware)
