project(tm_280 VERSION 1.0 LANGUAGES CXX C ASM)

add_compile_definitions(PSP_FW_VERSION=280 TM_PATH="/TM/280")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl)
