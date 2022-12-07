add_subdirectory(payloadex)

get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x040E0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payload_661_01g)
add_executable(tmctrl::payload_661_01g ALIAS payload_661_01g)

target_sources(payload_661_01g PRIVATE main.cpp)
target_link_libraries(payload_661_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::lowio iplsdk::syscon tm_common::common)
target_link_options(payload_661_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(payload_661_01g PRIVATE PSP_MODEL=0)

add_custom_command(OUTPUT payloadex_01g.h
    DEPENDS tmctrl::payloadex_661_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        payloadex="$<TARGET_FILE:payloadex_661_01g>".bin
        payloadex_01g.h
)

target_include_directories(payload_661_01g PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(payload_661_01g PRIVATE payloadex_01g.h)

add_custom_command(
    TARGET payload_661_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary payload_661_01g payload_661_01g.bin
    COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC>
        --input_payload payload_661_01g.bin
        --payload_addr 0x040E0000
        --ipl_addr 0x040EC000
        --payload_is_entry
        --output payload_01g.bin
)

add_executable(payload_661_02g)
add_executable(tmctrl::payload_661_02g ALIAS payload_661_02g)

target_sources(payload_661_02g PRIVATE main.cpp)
target_link_libraries(payload_661_02g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::lowio iplsdk::syscon tm_common::common)
target_link_options(payload_661_02g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(payload_661_02g PRIVATE PSP_MODEL=1)

add_custom_command(OUTPUT payloadex_02g.h
    DEPENDS tmctrl::payloadex_661_02g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        payloadex="$<TARGET_FILE:payloadex_661_02g>".bin
        payloadex_02g.h
)

target_include_directories(payload_661_02g PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(payload_661_02g PRIVATE payloadex_02g.h)

add_custom_command(
    TARGET payload_661_02g
    POST_BUILD
    COMMAND psp-objcopy -O binary payload_661_02g payload_661_02g.bin
    COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC>
        --input_payload payload_661_02g.bin
        --payload_addr 0x040E0000
        --ipl_addr 0x040EC000
        --payload_is_entry
        --output payload_02g.bin
)

add_executable(payload_661_03g)
add_executable(tmctrl::payload_661_03g ALIAS payload_661_03g)

target_sources(payload_661_03g PRIVATE main.cpp)
target_link_libraries(payload_661_03g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::lowio iplsdk::syscon tm_common::common)
target_link_options(payload_661_03g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(payload_661_03g PRIVATE PSP_MODEL=2)

add_custom_command(OUTPUT payloadex_03g.h
    DEPENDS tmctrl::payloadex_661_03g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        payloadex="$<TARGET_FILE:payloadex_661_03g>".bin
        payloadex_03g.h
)

target_include_directories(payload_661_03g PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(payload_661_03g PRIVATE payloadex_03g.h)

add_custom_command(
    TARGET payload_661_03g
    POST_BUILD
    COMMAND psp-objcopy -O binary payload_661_03g payload_661_03g.bin
    COMMAND $<TARGET_PROPERTY:make_ipl_payload,EXEC>
        --input_payload payload_661_03g.bin
        --payload_addr 0x040E0000
        --ipl_addr 0x040EC000
        --payload_is_entry
        --output payload_03g.bin
)