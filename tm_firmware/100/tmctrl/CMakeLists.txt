add_subdirectory(rebootex)

add_executable(tmctrl100)
build_prx(tmctrl100 EXPORTS exports.exp)

target_sources(tmctrl100 PRIVATE main.c flashemu.c)
target_compile_options(tmctrl100 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl100 PRIVATE utils pspdisplay_driver_weak pspreg_driver_weak pspsdk pspkernel)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex100_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex100_01g>".bin
        rebootex.h
)

target_sources(tmctrl100 PRIVATE rebootex.h)
target_include_directories(tmctrl100 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl100 PRIVATE lib)

add_custom_command(TARGET tmctrl100
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl100.prx
)
