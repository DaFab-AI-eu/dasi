include( GNUInstallDirs )

set( EXPORT_TARGETS_NAME ${PROJECT_NAME}Targets CACHE INTERNAL "Package exports" )

install( TARGETS ${PROJECT_NAME}
    EXPORT ${EXPORT_TARGETS_NAME}
    COMPONENT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )

install( FILES ${HEADER_FILE_LIST}
    COMPONENT ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} )

if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in )

    include( CMakePackageConfigHelpers )

    set( version_file ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake )
    write_basic_package_version_file( ${version_file} COMPATIBILITY AnyNewerVersion )

    set( cmake_package_destination_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME} )

    install( EXPORT ${EXPORT_TARGETS_NAME}
        COMPONENT ${PROJECT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        FILE ${EXPORT_TARGETS_NAME}.cmake
        DESTINATION ${cmake_package_destination_dir} )

    # package config file
    set( config_file ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake )
    configure_package_config_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        ${config_file} INSTALL_DESTINATION ${cmake_package_destination_dir} )

    install( FILES ${version_file} ${config_file}
        COMPONENT ${PROJECT_NAME}
        DESTINATION ${cmake_package_destination_dir} )

    # used for build directory
    export( EXPORT ${EXPORT_TARGETS_NAME}
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_TARGETS_NAME}.cmake )

endif( )
