get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FD0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex271_01g)
add_executable(tmctrl::rebootex271_01g ALIAS rebootex271_01g)
target_sources(rebootex271_01g PRIVATE main.cpp)
target_link_libraries(rebootex271_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex271_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex271_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex271_01g rebootex271_01g.bin
)
