project(tm_340 VERSION 1.0 LANGUAGES CXX ASM)

add_compile_definitions(TM_PATH="/TM/340OE")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl150)
add_subdirectory(tmctrl340)