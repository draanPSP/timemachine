add_library(payload_common STATIC)

target_sources(payload_common PRIVATE main.cpp config.cpp)

target_link_libraries(payload_common PRIVATE tm_common::common iplsdk::crt iplsdk::ff_ro iplsdk::lowio iplsdk::cache iplsdk::ctrl iplsdk::ms iplsdk::syscon iplsdk::serial)

set(LOAD_ADDR ${PAYLOAD_ENTRY_ADDR})
set(STACK_SIZE 0x2000)
get_target_property(LINKER_SCRIPT iplsdk::linker_script INTERFACE_SOURCES)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payload)

target_sources(payload PRIVATE performNandBoot.cpp performMsBoot.cpp)
target_compile_definitions(payload PRIVATE RESET_EXPLOIT)

target_link_libraries(payload PRIVATE payload_common tm_common::common tm_common::nobootrompatch iplsdk::cache iplsdk::ff_ro iplsdk::serial)
target_link_options(payload PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
        TARGET payload
        POST_BUILD
        COMMAND psp-objcopy -O binary payload payload.bin
)

set_target_properties(payload PROPERTIES OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/payload.bin)

add_executable(payload_legacy)

target_sources(payload_legacy PRIVATE performNandBoot_legacy.s performMsBoot.cpp)

target_link_libraries(payload_legacy PRIVATE payload_common tm_common::common tm_common::nobootrompatch iplsdk::cache iplsdk::ff_ro iplsdk::serial)
target_link_options(payload_legacy PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
        TARGET payload_legacy
        POST_BUILD
        COMMAND psp-objcopy -O binary payload_legacy payload_legacy.bin
)

set_target_properties(payload_legacy PROPERTIES OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/payload_legacy.bin)
