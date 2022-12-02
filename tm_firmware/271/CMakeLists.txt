project(tm_271 VERSION 1.0 LANGUAGES C CXX ASM)

add_compile_definitions(TM_PATH="/TM/271SE")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl150)
add_subdirectory(tmctrl271)