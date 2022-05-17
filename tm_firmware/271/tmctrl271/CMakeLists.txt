add_subdirectory(rebootex)

add_executable(tmctrl271)
build_prx(tmctrl271 EXPORTS exports.exp)

target_sources(tmctrl271 PRIVATE main.c)
target_compile_options(tmctrl271 PRIVATE -fshort-wchar -Wno-address-of-packed-member)
target_link_libraries(tmctrl271 PRIVATE utils tm_common::flash_emu pspsdk pspkernel271)
add_compile_definitions(PSP_FW_VERSION=271 TM_MS_PATH="ms0:/TM/271SE" FLASH_EMU_TOO_MANY_FILES_FIX FLASH_EMU_FLASH2)

add_custom_command(OUTPUT rebootex.h
    DEPENDS tmctrl::rebootex271_01g
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --gzip --c_style
        rebootex_01g="$<TARGET_FILE:rebootex271_01g>".bin
        rebootex.h
)

target_sources(tmctrl271 PRIVATE rebootex.h)
target_include_directories(tmctrl271 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_directories(tmctrl271 PRIVATE lib)

add_custom_command(TARGET tmctrl271
    POST_BUILD
    COMMAND psp-fixup-imports tmctrl271.prx
)
