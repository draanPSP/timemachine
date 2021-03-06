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
    100_tmctrl100.h
    100_payload.h
    100B_tmctrl100B.h
    100B_payload.h
    100B_vshex.h
    150_tmctrl150.h
    150_payload.h
    200_tmctrl200.h
    200_payload.h
    250_tmctrl250.h
    250_payload.h
    260_tmctrl260.h
    260_payload.h
    271_tmctrl150.h
    271_tmctrl271.h
    271_payload.h
    271_systemctrl150
    271_systemctrl
    271_paf.h
    271_vshmain.h
    280_tmctrl280.h
    280_payload.h
    340_tmctrl150.h
    340_tmctrl340.h
    340_payload.h
    340_systemctrl150
    340_systemctrl
    340_idcanager.h
    340_lcpatcher.h
    340_popcorn.h
    340_reboot150.h
    340_recovery.h
    340_systemctrl.h
    340_systemctrl150.h
    340_vshctrl.h
    )

target_link_libraries(timemachine PRIVATE z crypto stdc++fs)
target_include_directories(timemachine PRIVATE pspdecrypt ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(OUTPUT tm_sloader.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm_sloader="${CMAKE_SOURCE_DIR}"/../../build/tm_loader/tm_sloader.bin
        tm_sloader.h
)

add_custom_command(OUTPUT 100_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm100_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/100/ipl_payload/payload_01g.bin
        100_payload.h
)

add_custom_command(OUTPUT 100_tmctrl100.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm100_tmctrl100="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/100/tmctrl/tmctrl100.prx
        100_tmctrl100.h
)

add_custom_command(OUTPUT 100B_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm100B_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/100_Bogus/ipl_payload/payload_01g.bin
        100B_payload.h
)

add_custom_command(OUTPUT 100B_vshex.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm100B_vshex="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/100_Bogus/vshex/vshex.prx
        100B_vshex.h
)

add_custom_command(OUTPUT 100B_tmctrl100B.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm100B_tmctrl100B="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/100_Bogus/tmctrl/tmctrl100_Bogus.prx
        100B_tmctrl100B.h
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

add_custom_command(OUTPUT 260_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm260_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/260/ipl_payload/payload_01g.bin
        260_payload.h
)

add_custom_command(OUTPUT 260_tmctrl260.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm260_tmctrl260="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/260/tmctrl/tmctrl260.prx
        260_tmctrl260.h
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

add_custom_command(OUTPUT 280_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm280_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/280/ipl_payload/payload_01g.bin
        280_payload.h
)

add_custom_command(OUTPUT 280_tmctrl280.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm280_tmctrl280="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/280/tmctrl/tmctrl280.prx
        280_tmctrl280.h
)

add_custom_command(OUTPUT 340_payload.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_payload="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/340/ipl_payload/payload_01g.bin
        340_payload.h
)

add_custom_command(OUTPUT 340_tmctrl150.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_tmctrl150="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/340/tmctrl150/tmctrl150.prx
        340_tmctrl150.h
)

add_custom_command(OUTPUT 340_tmctrl340.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_tmctrl340="${CMAKE_SOURCE_DIR}"/../../build/tm_firmware/340/tmctrl340/tmctrl340.prx
        340_tmctrl340.h
)

add_custom_command(OUTPUT 340_idcanager.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_idcanager="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/idcanager.prx
        340_idcanager.h
)

add_custom_command(OUTPUT 340_lcpatcher.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_lcpatcher="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/lcpatcher.prx
        340_lcpatcher.h
)

add_custom_command(OUTPUT 340_popcorn.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_popcorn="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/popcorn.prx
        340_popcorn.h
)

add_custom_command(OUTPUT 340_reboot150.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_reboot150="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/reboot150.prx
        340_reboot150.h
)

add_custom_command(OUTPUT 340_recovery.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_recovery="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/recovery.prx
        340_recovery.h
)

add_custom_command(OUTPUT 340_systemctrl.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_systemctrl="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/systemctrl.prx
        340_systemctrl.h
)

add_custom_command(OUTPUT 340_systemctrl150.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_systemctrl150="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/systemctrl150.prx
        340_systemctrl150.h
)

add_custom_command(OUTPUT 340_vshctrl.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        tm340_vshctrl="${CMAKE_SOURCE_DIR}"/../../cfw_binaries/340OE/vshctrl.prx
        340_vshctrl.h
)