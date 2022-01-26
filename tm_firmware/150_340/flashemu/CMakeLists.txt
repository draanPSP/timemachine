add_executable(flashemu150_340)
build_prx(flashemu150_340 EXPORTS exports.exp)

target_sources(flashemu150_340 PRIVATE main.c flashemu.c)
target_compile_options(flashemu150_340 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(flashemu150_340 PRIVATE utils pspsdk pspkernel)

add_custom_command(TARGET flashemu150_340
    POST_BUILD
    COMMAND psp-fixup-imports flashemu150_340.prx
)
