macro( _set_verbose )

    message( STATUS "Project setting: ${ARGV0} = ${ARGV1}" )
    set( ${ARGV} )

endmacro( )

_set_verbose( BUILD_PYTHON      ON  CACHE   BOOL "Enable build pydasi"   FORCE )
_set_verbose( BUILD_EXAMPLES    OFF CACHE   BOOL "Enable build examples" FORCE )
_set_verbose( BUILD_TESTING     OFF CACHE   BOOL "Enable build tests"    FORCE )
