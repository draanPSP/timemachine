function(build_prx NAME)
    set(options OPTIONAL) # not used
    set(oneValueArgs EXPORTS)
    set(multiValueArgs CONFIGS) #not used
    cmake_parse_arguments(BUILD_PRX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_filename_component(outfile "${BUILD_PRX_EXPORTS}" NAME_WE)
    set(ExportsExp "${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_PRX_EXPORTS}")
    set(ExportsC "${outfile}.c")

    target_sources(${NAME} PRIVATE "${ExportsC}")

    add_custom_command(
        OUTPUT "${ExportsC}"
        DEPENDS "${ExportsExp}"
        COMMAND psp-build-exports -b ${ExportsExp} > ${ExportsC}
    )

    target_link_options(${NAME} PRIVATE -nostartfiles -specs=${PSPSDK}/lib/prxspecs -Wl,-q -T${PSPSDK}/lib/linkfile.prx)
    target_link_directories(${NAME} PRIVATE "${PSPDEV}/psp/sdk/lib")

    add_custom_command(TARGET ${NAME}
        POST_BUILD
        COMMAND psp-prxgen ${NAME} ${NAME}.prx
    )
endfunction()
