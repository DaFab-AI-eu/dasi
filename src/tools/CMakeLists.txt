include( GNUInstallDirs )

configure_file( dasi.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dasi @ONLY )

install( PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dasi DESTINATION ${CMAKE_INSTALL_BINDIR} )

foreach( _tool info list put get wipe policy schema )

    set( _tool_target dasi-${_tool} )

    add_executable( ${_tool_target} ${_tool_target}.cc )

    target_link_libraries( ${_tool_target} PRIVATE dasi eckit_option )

    install( TARGETS ${_tool_target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

endforeach()
