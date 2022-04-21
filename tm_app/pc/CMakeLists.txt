cmake_minimum_required(VERSION 3.16)

project(timemachine VERSION 1.0 LANGUAGES C CXX ASM)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(../../tools tools)

add_executable(timemachine)

add_subdirectory(pspdecrypt)
target_sources(timemachine 
  PRIVATE
    main.cpp
    tm_sloader.h
    150_tmctrl150.h
    150_payload.h
    200_tmctrl200.h
    200_payload.h
    250_tmctrl250.h
    250_payload.h
    271_tmctrl150.h
    271_tmctrl271.h
    271_payload.h
    271_systemctrl150
    271_systemctrl
    271_paf.h
    271_vshmain.h)

target_link_libraries(timemachine PRIVATE z crypto stdc++fs)
target_include_directories(timemachine PRIVATE pspdecrypt ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(OUTPUT tm_sloader.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm_sloader="${CMAKE_SOURCE_DIR}"/../../build/tm_loader/tm_sloader.bin
        tm_sloader.h
)

add_custom_command(OUTPUT 150_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm150_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/150/ipl_payload/payload_01g.bin
        150_payload.h
)

add_custom_command(OUTPUT 150_tmctrl150.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm150_tmctrl150="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/150/tmctrl/tmctrl150.prx
        150_tmctrl150.h
)

add_custom_command(OUTPUT 200_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm200_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/200/ipl_payload/payload_01g.bin
        200_payload.h
)

add_custom_command(OUTPUT 200_tmctrl200.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm200_tmctrl200="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/200/tmctrl/tmctrl200.prx
        200_tmctrl200.h
)

add_custom_command(OUTPUT 250_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm250_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/250/ipl_payload/payload_01g.bin
        250_payload.h
)

add_custom_command(OUTPUT 250_tmctrl250.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm250_tmctrl250="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/250/tmctrl/tmctrl250.prx
        250_tmctrl250.h
)

add_custom_command(OUTPUT 271_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/271/ipl_payload/payload_01g.bin
        271_payload.h
)

add_custom_command(OUTPUT 271_tmctrl150.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_tmctrl150="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/271/tmctrl150/tmctrl150.prx
        271_tmctrl150.h
)

add_custom_command(OUTPUT 271_tmctrl271.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_tmctrl271="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/271/tmctrl271/tmctrl271.prx
        271_tmctrl271.h
)

add_custom_command(OUTPUT 271_systemctrl150.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_systemctrl150="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/271SEC/systemctrl150.prx
        271_systemctrl150.h
)

add_custom_command(OUTPUT 271_systemctrl.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_systemctrl="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/271SEC/systemctrl.prx
        271_systemctrl.h
)

add_custom_command(OUTPUT 271_paf.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_paf="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/271SEC/paf.prx
        271_paf.h
)

add_custom_command(OUTPUT 271_vshmain.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm271_vshmain="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/271SEC/vshmain.prx
        271_vshmain.h
)
