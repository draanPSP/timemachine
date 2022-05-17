add_subdirectory(rebootex)

add_executable(tmctrl280)
build_prx(tmctrl280 EXPORTS exports.exp)

target_sources(tmctrl280 PRIVATE main.c)
target_compile_options(tmctrl280 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl280 PRIVATE utils tm_common::flash_emu tm_common::plainmodulespatch pspsdk pspkernel280)
add_compile_definitions(PSP_FW_VERSION=280 TM_MS_PATH="ms0:/TM/280")

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex280_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex280_01g>".bin
        rebootex.h
)

target_sources(tmctrl280 PRIVATE rebootex.h)
target_include_directories(tmctrl280 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl280 PRIVATE lib)

add_custom_command(TARGET tmctrl280
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl280.prx
)
