add_subdirectory(rebootex)

add_executable(tmctrl340)
build_prx(tmctrl340 EXPORTS exports.exp)

target_sources(tmctrl340 PRIVATE main.c)
target_compile_options(tmctrl340 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl340 PRIVATE utils tm_common::moduleutils tm_common::flash_emu pspsdk pspkernel340 SystemCtrlForKernel)
add_compile_definitions(PSP_FW_VERSION=340 FLASH_EMU_TOO_MANY_FILES_FIX)

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
