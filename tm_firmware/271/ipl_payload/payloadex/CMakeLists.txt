get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88700000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payloadex_271_01g)
add_executable(tmctrl::payloadex_271_01g ALIAS payloadex_271_01g)
target_sources(payloadex_271_01g PRIVATE main.cpp)
target_link_libraries(payloadex_271_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common)
target_link_options(payloadex_271_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET payloadex_271_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex_271_01g payloadex_271_01g.bin
)
