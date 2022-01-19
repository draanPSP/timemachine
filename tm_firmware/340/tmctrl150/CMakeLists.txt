add_subdirectory(rebootex)

add_executable(tmctrl340_150)
build_prx(tmctrl340_150 EXPORTS exports.exp)

target_sources(tmctrl340_150 PRIVATE main.c flashemu.c)
target_compile_options(tmctrl340_150 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl340_150 PRIVATE utils pspdisplay_driver_weak pspreg_driver_weak pspsdk pspkernel_sceIdStorage_weak)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex340_150_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex340_150_01g>".bin
        rebootex.h
)

target_sources(tmctrl340_150 PRIVATE rebootex.h)
target_include_directories(tmctrl340_150 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl340_150 PRIVATE lib)

add_custom_command(TARGET tmctrl340_150
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl340_150.prx
    COMMAND cp tmctrl340_150.prx tmctrl150.prx
)
