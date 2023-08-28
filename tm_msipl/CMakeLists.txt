project(tm_msipl VERSION 1.0 LANGUAGES C CXX ASM)

set(HEATSHRINK_WINDOW_SIZE 8)
set(HEATSHRINK_LOOKAHEAD_SIZE 4)
set(PAYLOAD_ENTRY_ADDR 0x40C0000)

add_subdirectory(loader)
add_subdirectory(payload)

add_custom_target(${PROJECT_NAME} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    COMMAND $<TARGET_PROPERTY:make_ms_ipl,EXEC> --window_sz2 ${HEATSHRINK_WINDOW_SIZE} --lookahead_sz2 ${HEATSHRINK_LOOKAHEAD_SIZE} --input_loader $<TARGET_PROPERTY:loader,OUTPUT_FILE> --input_payload $<TARGET_PROPERTY:payload,OUTPUT_FILE> --output ${PROJECT_NAME}.bin
    DEPENDS loader payload
)

add_custom_target(${PROJECT_NAME}_legacy ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_legacy.bin")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_legacy.bin"
    COMMAND $<TARGET_PROPERTY:make_ms_ipl,EXEC> --legacy --window_sz2 ${HEATSHRINK_WINDOW_SIZE} --lookahead_sz2 ${HEATSHRINK_LOOKAHEAD_SIZE} --input_loader $<TARGET_PROPERTY:loader_legacy,OUTPUT_FILE> --input_payload $<TARGET_PROPERTY:payload_legacy,OUTPUT_FILE> --output ${PROJECT_NAME}_legacy.bin
    DEPENDS loader_legacy payload_legacy
)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_legacy.bin DESTINATION release/ms_ipl)
