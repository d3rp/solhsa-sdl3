cmake_minimum_required(VERSION 3.24)

# Generic settings

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SUPPRESS_REGENERATION true) # Remove ZERO_CHECK from targets
set(CMAKE_WARN_DEPRECATED OFF)

set(PROJECT_NAME pathfinder)
set(PROJECT_VERSION 0.1.0)

# Options

#option(TESTS_ENABLED "Enable tests" OFF)

add_compile_definitions("$<$<CONFIG:Debug>:DEBUG=1>")
add_compile_definitions("$<$<NOT:$<CONFIG:Debug>>:NDEBUG>")

# Setup compiler options before including any subdirectories
if (APPLE)
    set(CMAKE_CXX_FLAGS "-Wextra -fconstexpr-steps=10000000")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

    set(RELEASE_FLAGS
            -O3
            # -fno-exceptions
            #-fno-honor-infinities
            #-fno-honor-nans
            #-fno-math-errno
            #-ffinite-math-only
            #-fassociative-math
            #-freciprocal-math
    )
    string(JOIN " " CMAKE_CXX_FLAGS_RELEASE ${RELEASE_FLAGS})
endif ()


########################
# PROJECT
########################

if (APPLE)
    project(${PROJECT_NAME}
            VERSION ${PROJECT_VERSION}
            LANGUAGES CXX OBJC OBJCXX)
else ()
    project(${PROJECT_NAME}
            VERSION ${PROJECT_VERSION}
            LANGUAGES C CXX)
endif ()

if (FORCE_DISABLE_CCACHE)
    message(STATUS "Ccached is forced off")
else()
    include("cmake/ccache.cmake")
    enable_ccache_if_available()
endif()


if (TESTS_ENABLED)

    message(STATUS "Tests \t\tENABLED")
    #enable_testing()
    #add_subdirectory(doctest)
    #add_subdirectory(test)
else ()
    message(STATUS "Tests \t\tDISABLED")
endif ()

# Add source files to target

set(HEADERS
)

set(SOURCES
        src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_sources(${PROJECT_NAME}
        PRIVATE
        ${HEADERS}
        ${SOURCES}
)

# Setup linking

#set(SDL_STATIC ON CACHE BOOL " " FORCE)
add_subdirectory(deps/sdl EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)
#target_compile_definitions(${PROJECT_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
# ...
# set some extra configs for each platform
#set_target_properties(${PROJECT_NAME}} PROPERTIES
#        # On macOS, make a proper .app bundle instead of a bare executable
#        MACOSX_BUNDLE TRUE
#        # Set the Info.plist file for Apple Mobile platforms. Without this file, your app
#        # will not launch.
#        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
#
#        # in Xcode, create a Scheme in the schemes dropdown for the app.
#        XCODE_GENERATE_SCHEME TRUE
#        # Identification for Xcode
#        XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER "com.d3rp.pathfinder"
#        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.d3rp.pathfinder"
#        XCODE_ATTRIBUTE_CURRENTYEAR "${CURRENTYEAR}"
#)
# Source file filters in IDE project

# Create a tree structure of all files in src dir
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
        ${HEADERS}
    )

