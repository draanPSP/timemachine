add_subdirectory(rebootex)

add_executable(tmctrl340)
build_prx(tmctrl340 EXPORTS exports.exp)

target_sources(tmctrl340 PRIVATE main.c flashemu.c)
target_compile_options(tmctrl340 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl340 PRIVATE utils pspsdk pspkernel340 SystemCtrlForKernel)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex340_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex340_01g>".bin
        rebootex.h
)

target_sources(tmctrl340 PRIVATE rebootex.h)
target_include_directories(tmctrl340 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl340 PRIVATE lib)

add_custom_command(TARGET tmctrl340
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl340.prx
)
