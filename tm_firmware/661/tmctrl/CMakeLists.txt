add_subdirectory(rebootex)

add_executable(tmctrl661)
build_prx(tmctrl661 EXPORTS exports.exp)

target_sources(tmctrl661 PRIVATE main.c)
target_compile_options(tmctrl661 PRIVATE -fno-pic -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl661 PRIVATE utils tm_common::moduleutils tm_common::flash_emu m33sdk pspsystemctrl_kernel pspkernel)
add_compile_definitions(FLASH_EMU_TOO_MANY_FILES_FIX M33_SDK)

add_custom_command(OUTPUT rebootex_01g.h
    DEPENDS tmctrl::rebootex661_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex661_01g>".bin
        rebootex_01g.h
)

add_custom_command(OUTPUT rebootex_02g.h
    DEPENDS tmctrl::rebootex661_02g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_02g="$<TARGET_FILE:rebootex661_02g>".bin
        rebootex_02g.h
)

target_sources(tmctrl661 PRIVATE rebootex_01g.h rebootex_02g.h)
target_include_directories(tmctrl661 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl661 PRIVATE lib)

add_custom_command(TARGET tmctrl661
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl661.prx
)
