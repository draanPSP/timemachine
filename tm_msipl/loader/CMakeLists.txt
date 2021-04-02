add_subdirectory(heatshrink)

# We handle the stack manually in crt0
set(STACK_SIZE 0x0)

get_target_property(LINKER_SCRIPT iplsdk::linker_script INTERFACE_SOURCES)


add_executable(loader)

target_sources(loader PRIVATE crt0.s main.cpp)
target_link_libraries(loader PRIVATE loader::heatshrink iplsdk::preipl iplsdk::lowio iplsdk::cache)
target_compile_definitions(loader PRIVATE RESET_EXPLOIT)
target_compile_definitions(loader PRIVATE PAYLOAD_ENTRY_ADDR=${PAYLOAD_ENTRY_ADDR})

set(LOAD_ADDR 0xBFC0001C)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linkerBFC.x"
)

target_link_options(loader PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linkerBFC.x)

add_custom_command(
        TARGET loader
        POST_BUILD
        COMMAND psp-objcopy -O binary loader loader.bin
)

set_target_properties(loader PROPERTIES OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/loader.bin)


add_executable(loader_legacy)

target_sources(loader_legacy PRIVATE crt0_legacy.s main.cpp)
target_link_libraries(loader_legacy PRIVATE loader::heatshrink iplsdk::preipl iplsdk::lowio iplsdk::cache)
target_compile_definitions(loader_legacy PRIVATE PAYLOAD_ENTRY_ADDR=${PAYLOAD_ENTRY_ADDR})

set(LOAD_ADDR 0xBFD00014)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linkerBFD.x"
)

target_link_options(loader_legacy PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linkerBFD.x)

add_custom_command(
        TARGET loader_legacy
        POST_BUILD
        COMMAND psp-objcopy -O binary loader_legacy loader_legacy.bin
)

set_target_properties(loader_legacy PROPERTIES OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/loader_legacy.bin)
