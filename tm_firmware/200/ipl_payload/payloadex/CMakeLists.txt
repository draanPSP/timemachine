get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x883e0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payloadex_200_01g)
add_executable(tmctrl::payloadex_200_01g ALIAS payloadex_200_01g)
target_sources(payloadex_200_01g PRIVATE main.cpp)
target_link_libraries(payloadex_200_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common tm_common::plainmodulespatch)
target_link_options(payloadex_200_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET payloadex_200_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex_200_01g payloadex_200_01g.bin
)
