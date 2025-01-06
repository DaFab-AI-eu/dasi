
list( APPEND dasi_test_srcs
    api_key
    api_simple_archive
    api_wipe
    api_purge
    api_query
    api_policydict
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
