add_library(tm_common_crt OBJECT)
add_library(tm_common::crt ALIAS tm_common_crt)

target_sources(tm_common_crt PRIVATE crt0.s)

add_library(tm_common_linker_script INTERFACE)
add_library(tm_common::linker_script ALIAS tm_common_linker_script)

target_sources(tm_common_linker_script INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/linker.x.in)
