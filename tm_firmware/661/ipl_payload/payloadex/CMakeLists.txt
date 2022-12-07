get_target_property(LINKER_SCRIPT tm_common::linker_script INTERFACE_SOURCES)

set(LOAD_ADDR 0x88FC0000)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(payloadex_661_01g)
add_executable(tmctrl::payloadex_661_01g ALIAS payloadex_661_01g)
target_sources(payloadex_661_01g PRIVATE main.cpp)
target_compile_options(payloadex_661_01g PRIVATE -Wno-main)
target_link_libraries(payloadex_661_01g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common tm661me_common::rebootpatches)
target_link_options(payloadex_661_01g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(payloadex_661_01g PRIVATE PSP_MODEL=0 PAYLOADEX)

add_custom_command(
    TARGET payloadex_661_01g
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex_661_01g payloadex_661_01g.bin
)

add_executable(payloadex_661_02g)
add_executable(tmctrl::payloadex_661_02g ALIAS payloadex_661_02g)
target_sources(payloadex_661_02g PRIVATE main.cpp)
target_compile_options(payloadex_661_02g PRIVATE -Wno-main)
target_link_libraries(payloadex_661_02g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common tm661me_common::rebootpatches)
target_link_options(payloadex_661_02g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(payloadex_661_02g PRIVATE PSP_MODEL=1 PAYLOADEX)

add_custom_command(
    TARGET payloadex_661_02g
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex_661_02g payloadex_661_02g.bin
)

add_executable(payloadex_661_03g)
add_executable(tmctrl::payloadex_661_03g ALIAS payloadex_661_03g)
target_sources(payloadex_661_03g PRIVATE main.cpp)
target_compile_options(payloadex_661_03g PRIVATE -Wno-main)
target_link_libraries(payloadex_661_03g PRIVATE tm_common::crt c utils iplsdk::cache iplsdk::ff_ro iplsdk::lowio iplsdk::ms tm_common::common tm661me_common::rebootpatches)
target_link_options(payloadex_661_03g PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(payloadex_661_03g PRIVATE PSP_MODEL=2 PAYLOADEX)

add_custom_command(
    TARGET payloadex_661_03g
    POST_BUILD
    COMMAND psp-objcopy -O binary payloadex_661_03g payloadex_661_03g.bin
)

