
list( APPEND _dasi_tests
    simple_archive
    wipe
    purge
    key
    query
#    policydict
)

foreach( _test ${_dasi_tests} )

    add_test(
        TARGET test_dasi_c_${_test}
        SOURCES test_${_test}.cc
        INCLUDES ${test_dasi_includes}
        LIBS dasi
    )

endforeach()

# TODO: Include some tests that don't pull in eckit, to prove that the API doesn't require it.
