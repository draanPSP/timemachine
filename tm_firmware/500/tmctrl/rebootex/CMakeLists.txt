# TODO: reverse this
add_library(rebootex500 INTERFACE)
add_library(tm_firmware::rebootex500 ALIAS rebootex500)

target_include_directories(rebootex500 INTERFACE include)
target_sources(rebootex500 INTERFACE include/rebootex_01g.h)
target_sources(rebootex500 INTERFACE include/rebootex_02g.h)