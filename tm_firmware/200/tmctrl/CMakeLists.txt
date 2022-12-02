add_subdirectory(rebootex)

add_executable(tmctrl200)
build_prx(tmctrl200 EXPORTS exports.exp)

target_sources(tmctrl200 PRIVATE main.c)
target_compile_options(tmctrl200 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl200 PRIVATE utils tm_common::moduleutils tm_common::flash_emu tm_common::plainmodulespatch pspreg_driver_weak pspsdk pspkernel)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex200_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex200_01g>".bin
        rebootex.h
)

target_sources(tmctrl200 PRIVATE rebootex.h)
target_include_directories(tmctrl200 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl200 PRIVATE lib)

add_custom_command(TARGET tmctrl200
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl200.prx
)
