add_subdirectory(rebootex)

add_executable(tmctrl500)
build_prx(tmctrl500 EXPORTS exports.exp)

target_sources(tmctrl500 PRIVATE main.c)
target_compile_options(tmctrl500 PRIVATE -fshort-wchar)
target_link_libraries(tmctrl500 PRIVATE m33sdk pspkernel pspsystemctrl_kernel tm_firmware::rebootex500)

add_custom_command(TARGET tmctrl500
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl500.prx
    COMMAND $<TARGET_PROPERTY:psppack,EXEC> --psptag 0x4C9416F0 --oetag 0xC6BA41D3 --devkitver 0x05000001 tmctrl500.prx tmctrl500.prx
)