cmake_minimum_required(VERSION 3.16)

project(timemachine VERSION 1.0 LANGUAGES C CXX ASM)

include(FetchContent)
include(cmake/BuildPrx.cmake)

set (CMAKE_CXX_STANDARD 20)

add_compile_options(-Os -fno-exceptions $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

FetchContent_Declare(
  iplsdk
  GIT_REPOSITORY https://github.com/draanPSP/iplsdk
  GIT_TAG        77c30f584b104f1c5484d92bf7319b8062c14819
)

FetchContent_MakeAvailable(iplsdk)

add_subdirectory(m33sdk)
add_subdirectory(utils)
add_subdirectory(tools)

add_subdirectory(tm_common)
add_subdirectory(tm_msipl)
add_subdirectory(tm_loader)
add_subdirectory(tm_firmware)
