project(tm_150 VERSION 1.0 LANGUAGES CXX ASM)

add_compile_definitions(PSP_FW_VERSION=150 TM_PATH="/TM/150")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl)
