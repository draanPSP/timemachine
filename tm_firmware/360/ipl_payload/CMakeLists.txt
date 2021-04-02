add_custom_target(${PROJECT_NAME}_ipl_payload ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/payload.bin")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/payload.bin"
    COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC> --payload_addr 0x040E0000 --ipl_addr 0x040F0000 --input_payload ${CMAKE_CURRENT_SOURCE_DIR}/dax_payload_02g.bin --output payload.bin
)
