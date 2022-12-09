project(tm_260 VERSION 1.0 LANGUAGES CXX C ASM)

add_compile_definitions(PSP_FW_VERSION=260 TM_PATH="/TM/260")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl)
