
list( APPEND dasi_test_srcs
    c_api_key
    c_api_simple_archive
    c_api_wipe
    c_api_purge
    c_api_query
)

foreach( _test ${dasi_test_srcs} )

    set( test_target test_dasi_${_test} )

    add_executable( ${test_target} test_${_test}.cc )
    target_include_directories( ${test_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${test_dasi_includes} )
    target_link_libraries( ${test_target} dasi )

    add_test( NAME test_dasi_${_test}
        COMMAND ${test_target}
    )

endforeach()
