get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex100_Bogus_01g)
add_executable(tmctrl::rebootex100_Bogus_01g ALIAS rebootex100_Bogus_01g)
target_sources(rebootex100_Bogus_01g PRIVATE main.cpp)
target_link_libraries(rebootex100_Bogus_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex100_Bogus_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex100_Bogus_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex100_Bogus_01g rebootex100_Bogus_01g.bin
)
