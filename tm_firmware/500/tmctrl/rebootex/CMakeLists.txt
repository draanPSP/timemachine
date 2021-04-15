get_target_property(LINKER_SCRIPT iplsdk::linker_script INTERFACE_SOURCES)

# No need to set stack pointer in rebootex
set(STACK_SIZE 0x0)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex500_01g)
add_executable(tmctrl::rebootex500_01g ALIAS rebootex500_01g)
target_sources(rebootex500_01g PRIVATE crt0.s main.cpp)
target_link_libraries(rebootex500_01g PRIVATE c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex500_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex500_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex500_01g rebootex500_01g.bin
)

add_executable(rebootex500_02g)
add_executable(tmctrl::rebootex500_02g ALIAS rebootex500_02g)
target_sources(rebootex500_02g PRIVATE crt0.s main.cpp)
target_link_libraries(rebootex500_02g PRIVATE c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex500_02g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex500_02g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex500_02g rebootex500_02g.bin
)

add_executable(rebootex500_03g)
add_executable(tmctrl::rebootex500_03g ALIAS rebootex500_03g)
target_sources(rebootex500_03g PRIVATE crt0.s main.cpp)
target_link_libraries(rebootex500_03g PRIVATE c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common)
target_link_options(rebootex500_03g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET rebootex500_03g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex500_03g rebootex500_03g.bin
)
