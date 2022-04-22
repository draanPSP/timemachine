add_executable(vshex_100_Bogus)
build_prx(vshex_100_Bogus EXPORTS exports.exp)

target_sources(vshex_100_Bogus PRIVATE main.c SystemCtrlForUser.S)
target_link_libraries(vshex_100_Bogus PRIVATE utils -lpsplibc -lpspuser)
target_link_directories(vshex_100_Bogus PRIVATE lib)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-pic")

add_custom_command(TARGET vshex_100_Bogus
    POST_BUILD
    COMMAND psp-fixup-imports vshex_100_Bogus.prx
    COMMAND cp vshex_100_Bogus.prx vshex.prx
)
