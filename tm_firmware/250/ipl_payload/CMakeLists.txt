add_subdirectory(payloadex)

get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x040E0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payload_250_01g)
add_executable(tmctrl::payload_250_01g ALIAS payload_250_01g)

target_sources(payload_250_01g PRIVATE main.cpp)
target_link_libraries(payload_250_01g PRIVATE tm_common::crt c utils tm_common::common)
target_link_options(payload_250_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(OUTPUT payloadex.h
    DEPENDS tmctrl::payloadex_250_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        payloadex="$<TARGET_FILE:payloadex_250_01g>".bin
        payloadex.h
)

target_include_directories(payload_250_01g PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(payload_250_01g PRIVATE payloadex.h)

add_custom_command(
    TARGET payload_250_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary payload_250_01g payload_250_01g.bin
    COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC>
        --input_payload payload_250_01g.bin
        --payload_addr 0x040E0000
        --ipl_addr 0x040F0000
        --output payload_01g.bin
)
