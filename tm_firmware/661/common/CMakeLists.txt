add_library(rebootpatches INTERFACE)
add_library(tm661me_common::rebootpatches ALIAS rebootpatches)

target_include_directories(rebootpatches INTERFACE include)
target_sources(rebootpatches INTERFACE rebootPatches.cpp)