get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(rebootex661_01g)
add_executable(tmctrl::rebootex661_01g ALIAS rebootex661_01g)
target_sources(rebootex661_01g PRIVATE main.cpp)
target_compile_options(rebootex661_01g PRIVATE -Wno-main -Wno-write-strings)
target_link_libraries(rebootex661_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common tm661me_common::rebootpatches)
target_link_options(rebootex661_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(rebootex661_01g PRIVATE PSP_MODEL=0 REBOOTEX)

add_custom_command(
    TARGET rebootex661_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex661_01g rebootex661_01g.bin
)

add_executable(rebootex661_02g)
add_executable(tmctrl::rebootex661_02g ALIAS rebootex661_02g)
target_sources(rebootex661_02g PRIVATE main.cpp)
target_compile_options(rebootex661_02g PRIVATE -Wno-main -Wno-write-strings)
target_link_libraries(rebootex661_02g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms iplsdk::syscon tm_common::common tm661me_common::rebootpatches)
target_link_options(rebootex661_02g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(rebootex661_02g PRIVATE PSP_MODEL=1 REBOOTEX)

add_custom_command(
    TARGET rebootex661_02g
    POST_BUILD
    COMMAND psp-objcopy -O binary rebootex661_02g rebootex661_02g.bin
)
