add_subdirectory(rebootex)

add_executable(tmctrl250)
build_prx(tmctrl250 EXPORTS exports.exp)

target_sources(tmctrl250 PRIVATE main.c flashemu.c)
target_compile_options(tmctrl250 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl250 PRIVATE utils tm_common::plainmodulespatch pspreg_driver_weak pspsdk pspkernel)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex250_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex250_01g>".bin
        rebootex.h
)

target_sources(tmctrl250 PRIVATE rebootex.h)
target_include_directories(tmctrl250 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl250 PRIVATE lib)

add_custom_command(TARGET tmctrl250
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl250.prx
)
