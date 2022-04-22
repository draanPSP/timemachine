get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88700000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payloadex_100_Bogus_01g)
add_executable(tmctrl::payloadex_100_Bogus_01g ALIAS payloadex_100_Bogus_01g)
target_sources(payloadex_100_Bogus_01g PRIVATE main.cpp)
target_link_libraries(payloadex_100_Bogus_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common)
target_link_options(payloadex_100_Bogus_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET payloadex_100_Bogus_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex_100_Bogus_01g payloadex_100_Bogus_01g.bin
)
