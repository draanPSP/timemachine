project(tools VERSION 1.0)

add_custom_target(bin2cpp ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bin2cpp/bin2cpp.py
)

set_property(TARGET bin2cpp PROPERTY EXEC ${CMAKE_CURRENT_SOURCE_DIR}/bin2cpp/bin2cpp.py)

add_custom_target(psppack ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/psppack/psppack.py
)

set_property(TARGET psppack PROPERTY EXEC ${CMAKE_CURRENT_SOURCE_DIR}/psppack/psppack.py)

add_custom_target(make_ms_ipl ALL
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/psptools/make_ms_ipl.py
)

set_property(TARGET make_ms_ipl PROPERTY EXEC ${CMAKE_CURRENT_SOURCE_DIR}/psptools/make_ms_ipl.py)

add_custom_target(make_ipl_payload ALL
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/psptools/make_ipl_payload.py
)

set_property(TARGET make_ipl_payload PROPERTY EXEC ${CMAKE_CURRENT_SOURCE_DIR}/psptools/make_ipl_payload.py)
