project(tm_150_340 VERSION 1.0 LANGUAGES CXX ASM)

add_compile_definitions(TM_PATH="/TM/150_340")

add_subdirectory(ipl_payload)
add_subdirectory(flashemu)
add_subdirectory(tmctrl)
