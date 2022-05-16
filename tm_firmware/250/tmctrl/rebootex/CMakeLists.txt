get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex250_01g)
add_executable(tmctrl::rebootex250_01g ALIAS rebootex250_01g)
target_sources(rebootex250_01g PRIVATE main.cpp)
target_link_libraries(rebootex250_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon iplsdk::serial tm_common::common  tm_common::plainmodulespatch)
target_link_options(rebootex250_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
add_custom_command(
    TARGET rebootex250_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex250_01g rebootex250_01g.bin
)
