get_target_property(LINKER_SCRIPT iplsdk::linker_script INTERFACE_SOURCES)

# No need to set stack pointer in rebootex
set(STACK_SIZE 0x0)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex150_01g)
add_executable(tmctrl::rebootex150_01g ALIAS rebootex150_01g)
target_sources(rebootex150_01g PRIVATE crt0.s main.cpp)
target_link_libraries(rebootex150_01g PRIVATE c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex150_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex150_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex150_01g rebootex150_01g.bin
)
