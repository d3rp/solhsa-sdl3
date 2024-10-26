
function (enable_ccache_if_available)

    # Use ccache if available
    find_program(CCACHE_PROGRAM ccache)
    mark_as_advanced(FORCE CCACHE_PROGRAM)

    find_program(SCCACHE_PROGRAM sccache)
    mark_as_advanced(FORCE SCCACHE_PROGRAM)

    # See info on the README.md file
    # Prefer sccache if both ccache and sccache are found because Windows users may have
    # ccache installed with MinGW which would not work with MSVC.
    if (SCCACHE_PROGRAM)
        message(STATUS "Found sccache: ${SCCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    elseif (CCACHE_PROGRAM)
        message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
        set(ENV{CCACHE_MAXSIZE} 10G)
        set(ENV{CCACHE_CPP2} true)
        set(ENV{CCACHE_HARDLINK} true)
    #    set(ENV{CCACHE_SLOPPINESS} "fhile_macro,time_macros,include_file_mtime,include_file_ctime,file_stat_matches")
        if (CMAKE_GENERATOR MATCHES "Ninja")
            set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
        else ()
            set_property(GLOBAL PROPERTY CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
            set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        endif ()
    else ()
        message(STATUS "Could NOT find ccache nor sccache, no compiler caching enabled")
    endif ()

endfunction()