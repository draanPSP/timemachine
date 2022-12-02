add_subdirectory(rebootex)

add_executable(tmctrl260)
build_prx(tmctrl260 EXPORTS exports.exp)

target_sources(tmctrl260 PRIVATE main.c)
target_compile_options(tmctrl260 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl260 PRIVATE utils tm_common::moduleutils tm_common::flash_emu tm_common::plainmodulespatch pspreg_driver_weak pspsdk pspkernel)
add_compile_definitions(PSP_FW_VERSION=260 TM_MS_PATH="ms0:/TM/260")

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex260_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex260_01g>".bin
        rebootex.h
)

target_sources(tmctrl260 PRIVATE rebootex.h)
target_include_directories(tmctrl260 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl260 PRIVATE lib)

add_custom_command(TARGET tmctrl260
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl260.prx
)
