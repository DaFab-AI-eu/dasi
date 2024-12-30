macro( _set_verbose )

    message( STATUS "Project setting: ${ARGV0} = ${ARGV1}" )
    set( ${ARGV} )

endmacro( )

_set_verbose( ENABLE_TESTING    OFF CACHE   BOOL "Enable build testing" FORCE)
