add_library(common INTERFACE)
add_library(tm_common::common ALIAS common)

target_include_directories(common INTERFACE include)
target_sources(common INTERFACE include/tm_common.h)
target_link_libraries(common INTERFACE iplsdk::hw)

add_library(nobootrompatch STATIC)
add_library(tm_common::nobootrompatch ALIAS nobootrompatch)

target_include_directories(nobootrompatch PUBLIC include)

target_sources(nobootrompatch PRIVATE noBootromPatch.cpp)

target_link_libraries(nobootrompatch PRIVATE tm_common::common iplsdk::serial)
