add_subdirectory(payloadex)

get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x040D6000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

foreach(GEN 1 2 3)
    math(EXPR MODEL "${GEN}-1")
    set(EXEC_NAME payload_661_0${GEN}g)
    add_executable(${EXEC_NAME})
    add_executable(tmctrl::${EXEC_NAME} ALIAS ${EXEC_NAME})
    target_sources(${EXEC_NAME} PRIVATE main.cpp)
    target_link_libraries(${EXEC_NAME} PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::lowio iplsdk::syscon tm_common::common)
    target_link_options(${EXEC_NAME} PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
    target_compile_definitions(${EXEC_NAME} PRIVATE PSP_MODEL=${MODEL})

    add_custom_command(
        OUTPUT payloadex_0${GEN}g.h
        DEPENDS tmctrl::payloadex_661_0${GEN}g
        COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
            --c_style
            payloadex="$<TARGET_FILE:payloadex_661_0${GEN}g>".bin
            payloadex_0${GEN}g.h
    )

    target_include_directories(${EXEC_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(${EXEC_NAME} PRIVATE payloadex_0${GEN}g.h)
    
    add_custom_command(
        TARGET ${EXEC_NAME}
        POST_BUILD
        COMMAND psp-objcopy -O binary ${EXEC_NAME} ${EXEC_NAME}.bin
        COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC>
            --input_payload ${EXEC_NAME}.bin
            --payload_addr 0x040D6000
            --ipl_addr 0x040EC000
            --output payload_0${GEN}g.bin
    )
endforeach()
