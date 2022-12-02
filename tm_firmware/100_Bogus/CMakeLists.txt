project(tm_100 VERSION 1.0 LANGUAGES C CXX ASM)

add_compile_definitions(PSP_FW_VERSION=100 TM_PATH="/TM/100_Bogus")

add_subdirectory(ipl_payload)
add_subdirectory(tmctrl)
add_subdirectory(vshex)
