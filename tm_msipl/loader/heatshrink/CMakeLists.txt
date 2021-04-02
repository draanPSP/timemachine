add_library(heatshrink STATIC)
add_library(loader::heatshrink ALIAS heatshrink)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/heatshrink_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/heatshrink_config.h"
)

target_include_directories(heatshrink PUBLIC include)
target_include_directories(heatshrink PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_sources(heatshrink PRIVATE heatshrink_decoder.c)

target_link_libraries(heatshrink PRIVATE c)