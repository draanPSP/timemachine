project(tm_200 VERSION 1.0 LANGUAGES CXX C ASM)

add_compile_definitions(PSP_FW_VERSION=200 TM_PATH="/TM/200")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl)
