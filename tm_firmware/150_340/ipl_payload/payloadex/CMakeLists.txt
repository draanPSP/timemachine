get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88700000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payloadex340_150)
add_executable(tmctrl::payloadex340_150 ALIAS payloadex340_150)
target_sources(payloadex340_150 PRIVATE main.cpp)
target_link_libraries(payloadex340_150 PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common)
target_link_options(payloadex340_150 PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET payloadex340_150
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex340_150 payloadex340_150.bin
)
