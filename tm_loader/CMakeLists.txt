project(tm_loader VERSION 1.0 LANGUAGES C CXX ASM)

set(LOAD_ADDR 0x04000000)
set(STACK_SIZE 0x2000)
get_target_property(LINKER_SCRIPT iplsdk::linker_script INTERFACE_SOURCES)

configure_file(
    "${LINKER_SCRIPT}"
    "${CMAKE_CURRENT_BINARY_DIR}/linker.x"
)

add_executable(tm_mloader)

target_sources(tm_mloader PRIVATE main.cpp)
target_link_libraries(tm_mloader PRIVATE c tm_common::common tm_common::nobootrompatch iplsdk::cache iplsdk::crt iplsdk::lowio iplsdk::ff_ro iplsdk::ms iplsdk::serial)
target_link_options(tm_mloader PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)
target_compile_definitions(tm_mloader PRIVATE MLOADER)

add_custom_command(
    TARGET tm_mloader
    POST_BUILD
    COMMAND psp-objcopy -O binary tm_mloader tm_mloader.bin
)

add_executable(tm_sloader)

target_sources(tm_sloader PRIVATE main.cpp)
target_link_libraries(tm_sloader PRIVATE c tm_common::common tm_common::nobootrompatch iplsdk::cache iplsdk::crt iplsdk::lowio iplsdk::ff_ro iplsdk::ms iplsdk::serial)
target_link_options(tm_sloader PRIVATE -nostdlib -T${CMAKE_CURRENT_BINARY_DIR}/linker.x)

add_custom_command(
    TARGET tm_sloader
    POST_BUILD
    COMMAND psp-objcopy -O binary tm_sloader tm_sloader.bin
)
