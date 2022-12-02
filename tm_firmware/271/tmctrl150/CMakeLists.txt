add_executable(tmctrl271_150)
build_prx(tmctrl271_150 EXPORTS exports.exp)

target_sources(tmctrl271_150 PRIVATE main.c)
target_compile_options(tmctrl271_150 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl271_150 PRIVATE utils tm_common::moduleutils tm_common::flash_emu pspdisplay_driver_weak pspreg_driver_weak pspsdk pspkernel_sceIdStorage_weak)
add_compile_definitions(PSP_FW_VERSION=150 TM_MS_PATH="ms0:/TM/271SE")

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex271_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex271_01g>".bin
        rebootex.h
)

target_sources(tmctrl271_150 PRIVATE rebootex.h)
target_include_directories(tmctrl271_150 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl271_150 PRIVATE lib)

add_custom_command(TARGET tmctrl271_150
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl271_150.prx
    COMMAND cp tmctrl271_150.prx tmctrl150.prx
)
