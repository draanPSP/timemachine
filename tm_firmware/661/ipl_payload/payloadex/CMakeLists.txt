get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

foreach(GEN 1 2 3)
    math(EXPR MODEL "${GEN}-1")
    set(EXEC_NAME payloadex_661_0${GEN}g)
    add_executable(${EXEC_NAME})
    add_executable(tmctrl::${EXEC_NAME} ALIAS ${EXEC_NAME})
    target_sources(${EXEC_NAME} PRIVATE main.cpp)
    target_compile_options(${EXEC_NAME} PRIVATE -Wno-main)
    target_link_libraries(${EXEC_NAME} PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common tm661me_common::rebootpatches)
    target_link_options(${EXEC_NAME} PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
    target_compile_definitions(${EXEC_NAME} PRIVATE PSP_MODEL=${MODEL} PAYLOADEX)

    add_custom_command(
        TARGET ${EXEC_NAME}
        POST_BUILD
        COMMAND psp-objcopy -O binary ${EXEC_NAME} ${EXEC_NAME}.bin
    )
endforeach()