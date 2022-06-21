add_subdirectory(rebootex)

add_executable(tmctrl150_340)
build_prx(tmctrl150_340 EXPORTS exports.exp)

target_sources(tmctrl150_340 PRIVATE main.c)
target_compile_options(tmctrl150_340 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl150_340 PRIVATE utils pspreg_driver_weak pspsdk pspkernel)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex150_340
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex="$<TARGET_FILE:rebootex150_340>".bin
        rebootex.h
)

target_sources(tmctrl150_340 PRIVATE rebootex.h)
target_include_directories(tmctrl150_340 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl150_340 PRIVATE lib)

add_custom_command(TARGET tmctrl150_340
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl150_340.prx
)
