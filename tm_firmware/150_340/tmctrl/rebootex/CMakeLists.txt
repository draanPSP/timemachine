get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex150_340)
add_executable(tmctrl::rebootex150_340 ALIAS rebootex150_340)
target_sources(rebootex150_340 PRIVATE main.cpp)
target_link_libraries(rebootex150_340 PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex150_340 PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex150_340
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex150_340 rebootex150_340.bin
)
