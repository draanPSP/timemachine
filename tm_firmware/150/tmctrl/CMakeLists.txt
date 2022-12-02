add_subdirectory(rebootex)

add_executable(tmctrl150)
build_prx(tmctrl150 EXPORTS exports.exp)

target_sources(tmctrl150 PRIVATE main.c)
target_compile_options(tmctrl150 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl150 PRIVATE tm_common::moduleutils tm_common::flash_emu pspdisplay_driver_weak pspreg_driver_weak pspsdk pspkernel)
add_compile_definitions(PSP_FW_VERSION=150 TM_MS_PATH="ms0:/TM/150")

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex150_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex150_01g>".bin
        rebootex.h
)

target_sources(tmctrl150 PRIVATE rebootex.h)
target_include_directories(tmctrl150 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl150 PRIVATE lib)

add_custom_command(TARGET tmctrl150
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl150.prx
)
