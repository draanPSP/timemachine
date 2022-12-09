project(tm_250 VERSION 1.0 LANGUAGES CXX C ASM)

add_compile_definitions(PSP_FW_VERSION=250 TM_PATH="/TM/250")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl)
