cmake_minimum_required(VERSION 3.16)

project(timemachine VERSION 1.0 LANGUAGES C CXX ASM)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(../../tools tools)

SET(BUILD_DIR ${CMAKE_SOURCE_DIR}/../../build/)
SET(TM_FW_DIR ${BUILD_DIR}tm_firmware/)
SET(CFW_FILES_DIR ${CMAKE_SOURCE_DIR}/../../cfw_binaries/)

set(TM_FILES
    # Common
    tm_sloader ${BUILD_DIR}tm_loader/tm_sloader.bin
    tm_mloader ${BUILD_DIR}tm_loader/tm_mloader.bin
    
    # 1.00
    tm100_tmctrl100 ${TM_FW_DIR}100/tmctrl/tmctrl100.prx
    tm100_payload ${TM_FW_DIR}100/ipl_payload/payload_01g.bin
    
    # 1.00 Bogus
    tm100B_tmctrl100B ${TM_FW_DIR}100_Bogus/tmctrl/tmctrl100_Bogus.prx
    tm100B_payload ${TM_FW_DIR}100_Bogus/ipl_payload/payload_01g.bin
    tm100B_vshex ${TM_FW_DIR}100_Bogus/vshex/vshex.prx
    
    # 1.50
    tm150_tmctrl150 ${TM_FW_DIR}150/tmctrl/tmctrl150.prx
    tm150_payload ${TM_FW_DIR}150/ipl_payload/payload_01g.bin
    
    # 2.00
    tm200_tmctrl200 ${TM_FW_DIR}200/tmctrl/tmctrl200.prx
    tm200_payload ${TM_FW_DIR}200/ipl_payload/payload_01g.bin
    
    # 2.50
    tm250_tmctrl250 ${TM_FW_DIR}250/tmctrl/tmctrl250.prx
    tm250_payload ${TM_FW_DIR}250/ipl_payload/payload_01g.bin
    
    # 2.60
    tm260_tmctrl260 ${TM_FW_DIR}260/tmctrl/tmctrl260.prx
    tm260_payload ${TM_FW_DIR}260/ipl_payload/payload_01g.bin
    
    # 2.71 SE-C
    tm271_tmctrl150 ${TM_FW_DIR}271/tmctrl150/tmctrl150.prx
    tm271_tmctrl271 ${TM_FW_DIR}271/tmctrl271/tmctrl271.prx
    tm271_payload ${TM_FW_DIR}271/ipl_payload/payload_01g.bin
    tm271_systemctrl150 ${CFW_FILES_DIR}271SEC/systemctrl150.prx
    tm271_systemctrl ${CFW_FILES_DIR}271SEC/systemctrl.prx
    tm271_paf ${CFW_FILES_DIR}271SEC/paf.prx
    tm271_vshmain ${CFW_FILES_DIR}271SEC/vshmain.prx
    
    # 1.50
    tm280_tmctrl280 ${TM_FW_DIR}280/tmctrl/tmctrl280.prx
    tm280_payload ${TM_FW_DIR}280/ipl_payload/payload_01g.bin

    # 3.40 OE-A
    tm340_tmctrl150 ${TM_FW_DIR}340/tmctrl150/tmctrl150.prx
    tm340_tmctrl340 ${TM_FW_DIR}340/tmctrl340/tmctrl340.prx
    tm340_payload ${TM_FW_DIR}340/ipl_payload/payload_01g.bin
    tm340_idcanager ${CFW_FILES_DIR}340OE/idcanager.prx
    tm340_lcpatcher ${CFW_FILES_DIR}340OE/lcpatcher.prx
    tm340_popcorn ${CFW_FILES_DIR}340OE/popcorn.prx
    tm340_reboot150 ${CFW_FILES_DIR}340OE/reboot150.prx
    tm340_recovery ${CFW_FILES_DIR}340OE/recovery.prx
    tm340_systemctrl150 ${CFW_FILES_DIR}340OE/systemctrl150.prx
    tm340_systemctrl ${CFW_FILES_DIR}340OE/systemctrl.prx
    tm340_vshctrl ${CFW_FILES_DIR}340OE/vshctrl.prx

    # 6.61 ME
    tm661_tmctrl661 ${TM_FW_DIR}661/tmctrl/tmctrl661.prx
    tm661_payload_01g ${TM_FW_DIR}661/ipl_payload/payload_01g.bin
    tm661_payload_02g ${TM_FW_DIR}661/ipl_payload/payload_02g.bin
    tm661_payload_03g ${TM_FW_DIR}661/ipl_payload/payload_03g.bin
    tm661_configme ${CFW_FILES_DIR}661ME/config.me
    tm661_dax9660 ${CFW_FILES_DIR}661ME/dax9660.prx
    tm661_dcman ${CFW_FILES_DIR}661ME/dcman.prx
    tm661_emc_sm_updater ${CFW_FILES_DIR}661ME/emc_sm_updater.prx
    tm661_horoscope ${CFW_FILES_DIR}661ME/horoscope.prx
    tm661_idmanager ${CFW_FILES_DIR}661ME/idmanager.prx
    tm661_inferno ${CFW_FILES_DIR}661ME/inferno.prx
    tm661_iop ${CFW_FILES_DIR}661ME/iop.prx
    tm661_ipl_update ${CFW_FILES_DIR}661ME/ipl_update.prx
    tm661_isotope ${CFW_FILES_DIR}661ME/isotope.prx
    tm661_lfatfs_updater ${CFW_FILES_DIR}661ME/lfatfs_updater.prx
    tm661_lflash_fatfmt_updater ${CFW_FILES_DIR}661ME/lflash_fatfmt_updater.prx
    tm661_libpsardumper ${CFW_FILES_DIR}661ME/libpsardumper.prx
    tm661_pspbtdnf ${CFW_FILES_DIR}661ME/pspbtdnf.bin
    tm661_pspbtdnf_02g ${CFW_FILES_DIR}661ME/pspbtdnf_02g.bin
    tm661_pspbtjnf ${CFW_FILES_DIR}661ME/pspbtjnf.bin
    tm661_pspbtjnf_02g ${CFW_FILES_DIR}661ME/pspbtjnf_02g.bin
    tm661_pspbtjnf_03g ${CFW_FILES_DIR}661ME/pspbtjnf_03g.bin
    tm661_pspdecrypt ${CFW_FILES_DIR}661ME/pspdecrypt.prx
    tm661_pulsar ${CFW_FILES_DIR}661ME/pulsar.prx
    tm661_recovery ${CFW_FILES_DIR}661ME/recovery.prx
    tm661_resurrection ${CFW_FILES_DIR}661ME/resurrection.prx
    tm661_satellite ${CFW_FILES_DIR}661ME/satellite.prx
    tm661_systemctrl_01g ${CFW_FILES_DIR}661ME/systemctrl_01g.prx
    tm661_systemctrl_02g ${CFW_FILES_DIR}661ME/systemctrl_02g.prx
    tm661_systemctrl_03g ${CFW_FILES_DIR}661ME/systemctrl_03g.prx
    tm661_usbdev ${CFW_FILES_DIR}661ME/usbdev.prx
    tm661_vshctrl_02g ${CFW_FILES_DIR}661ME/vshctrl_02g.prx
)

list(LENGTH TM_FILES tm_files_len)
math(EXPR tm_files_len "${tm_files_len}/2 - 1")

set(SRC_FILES main.cpp)

foreach(i RANGE ${tm_files_len})

    math(EXPR name_ind "${i}*2")
    math(EXPR path_index "${i}*2+1")
    list(GET TM_FILES ${name_ind} TM_FILE)
    list(GET TM_FILES ${path_index} TM_FILE_PATH)

    list(APPEND SRC_FILES ${TM_FILE}.h)
    
    add_custom_command(OUTPUT ${TM_FILE}.h
    COMMAND $<TARGET_PROPERTY:bin2cpp,EXEC>
        --c_style
        ${TM_FILE}=${TM_FILE_PATH}
        ${TM_FILE}.h
    )
endforeach()

add_executable(timemachine)
add_subdirectory(pspdecrypt)
target_sources(timemachine PRIVATE ${SRC_FILES})
target_link_libraries(timemachine PRIVATE z crypto stdc++fs)
target_include_directories(timemachine PRIVATE pspdecrypt ${CMAKE_CURRENT_BINARY_DIR})