add_subdirectory(payloadex)

get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x040E0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payload340_150)
add_executable(tmctrl::payload340_150 ALIAS payload340_150)

target_sources(payload340_150 PRIVATE main.cpp)
target_link_libraries(payload340_150 PRIVATE tm_common::crt c utils tm_common::common)
target_link_options(payload340_150 PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(OUTPUT payloadex.h
    DEPENDS tmctrl::payloadex340_150
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        payloadex="$<TARGET_FILE:payloadex340_150>".bin
        payloadex.h
)

target_include_directories(payload340_150 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(payload340_150 PRIVATE payloadex.h)

add_custom_command(
    TARGET payload340_150
    POST_BUILD
    COMMAND psp-objcopy -O binary payload340_150 payload340_150.bin
    COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC>
        --input_payload payload340_150.bin
        --payload_addr 0x040E0000
        --ipl_addr 0x040F0000
        --output payload.bin
)
