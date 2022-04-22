add_subdirectory(rebootex)

add_executable(tmctrl100_Bogus)
build_prx(tmctrl100_Bogus EXPORTS exports.exp)

target_sources(tmctrl100_Bogus PRIVATE main.c flashemu.c)
target_compile_options(tmctrl100_Bogus PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl100_Bogus PRIVATE utils pspreg_driver_weak pspsdk pspkernel)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex100_Bogus_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex100_Bogus_01g>".bin
        rebootex.h
)

target_sources(tmctrl100_Bogus PRIVATE rebootex.h)
target_include_directories(tmctrl100_Bogus PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl100_Bogus PRIVATE lib)

add_custom_command(TARGET tmctrl100_Bogus
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl100_Bogus.prx
)
