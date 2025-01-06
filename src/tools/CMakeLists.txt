
configure_file( dasi.in ${CMAKE_BINARY_DIR}/bin/dasi @ONLY )

foreach( _tool info list put get wipe policy schema )

    set( DASI_TOOL_TARGET dasi-${_tool} )

    add_executable( ${DASI_TOOL_TARGET} ${DASI_TOOL_TARGET}.cc )

    target_link_libraries( ${DASI_TOOL_TARGET} PRIVATE dasi eckit_option )

    install( TARGETS ${DASI_TOOL_TARGET} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

endforeach()
