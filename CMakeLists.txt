cmake_minimum_required(VERSION 3.15...3.23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "macOS deployment target (Apple clang only)")

project(oxen-logging VERSION 1.0.3 LANGUAGES CXX)

option(OXEN_LOGGING_WARNINGS_AS_ERRORS "treat all warnings as errors. turn off for development, on for release" OFF)
option(OXEN_LOGGING_FORCE_SUBMODULES "Force use of the bundled fmt/spdlog rather than looking for system packages" OFF)
option(OXEN_LOGGING_RELEASE_TRACE "Enable trace logging in release builds" OFF)
option(OXEN_LOGGING_FMT_HEADER_ONLY "Use fmt in header-only mode" OFF)
option(OXEN_LOGGING_SPDLOG_HEADER_ONLY "Use spdlog in header-only mode" OFF)

if(NOT OXEN_LOGGING_FORCE_SUBMODULES)
    if(NOT TARGET fmt::fmt)
        find_package(fmt 9.0.0 CONFIG QUIET)
        if(fmt_FOUND)
            message(STATUS "Found fmt ${fmt_VERSION}")
            # Make the target available to the parent project (which is the case if we go by
            # subproject, but isn't for packages we find via find_package).  cmake 3.24+ has a
            # `GLOBAL` flag in the find_package, but we need this to work on older cmakes as well.
            set_target_properties(fmt::fmt PROPERTIES IMPORTED_GLOBAL TRUE)
        else()
            message(STATUS "Did not find suitable fmt; using submodule")
        endif()
    endif()
    set(min_spdlog 1.9.1)
    if(NOT TARGET fmt::fmt OR fmt_VERSION VERSION_GREATER_EQUAL 10.0.0)
        set(min_spdlog 1.12.0)
    endif()

    if(NOT TARGET spdlog::spdlog)
        find_package(spdlog ${min_spdlog} CONFIG QUIET)
        if(spdlog_FOUND)
            message(STATUS "Found spdlog ${spdlog_VERSION}")
            # Make available in parent; see above.
            set_target_properties(spdlog::spdlog PROPERTIES IMPORTED_GLOBAL TRUE)
        else()
            message(STATUS "Did not find suitable spdlog; using submodule")
        endif()
    endif()
endif()

set(OXEN_LOGGING_FMT_TARGET fmt::fmt)
set(OXEN_LOGGING_SPDLOG_TARGET spdlog::spdlog)

if(NOT TARGET fmt::fmt)
    if(OXEN_LOGGING_FMT_HEADER_ONLY)
        set(OXEN_LOGGING_FMT_TARGET fmt::fmt-header-only)
    endif()
    add_subdirectory(fmt)
endif()

if(NOT TARGET spdlog::spdlog)
    if(OXEN_LOGGING_FMT_HEADER_ONLY)
        set(SPDLOG_FMT_EXTERNAL_HO ON CACHE INTERNAL "")
    else()
        set(SPDLOG_FMT_EXTERNAL ON CACHE INTERNAL "")
    endif()
    if(OXEN_LOGGING_SPDLOG_HEADER_ONLY)
        set(OXEN_LOGGING_SPDLOG_TARGET spdlog::spdlog_header_only)
    endif()
    add_subdirectory(spdlog)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(oxen-logging STATIC
    src/catlogger.cpp
    src/level.cpp
    src/log.cpp
    src/type.cpp
)

target_include_directories(oxen-logging PUBLIC include)
target_link_libraries(oxen-logging PUBLIC ${OXEN_LOGGING_FMT_TARGET} ${OXEN_LOGGING_SPDLOG_TARGET})
target_compile_features(oxen-logging PUBLIC cxx_std_20)

set(warning_flags -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-function -Werror=vla)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND warning_flags -Wno-unknown-warning-option)
endif()
if (OXEN_LOGGING_WARNINGS_AS_ERRORS)
    list(APPEND warning_flags -Werror)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13 AND fmt_VERSION VERSION_LESS 10.0.1)
    list(APPEND warning_flags -Wno-error=dangling-reference)
endif()

add_library(oxen-logging-warnings INTERFACE)

if(NOT MSVC)
    target_compile_options(oxen-logging-warnings INTERFACE "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${warning_flags}>")
endif()
target_link_libraries(oxen-logging PRIVATE oxen-logging-warnings)

set(oxen_logging_source_roots "" CACHE INTERNAL "")
target_compile_definitions(oxen-logging PUBLIC OXEN_LOGGING_SOURCE_ROOTS= OXEN_LOGGING_SOURCE_ROOTS_LEN=0)

# Prepend a source path to the source roots to be stripped from log statements.  The given path is
# added *first* in the list of paths, and any existing duplicates are removed (so calling it
# multiple times will not add it multiple times, but *will* shift it to the front of the list).
function(oxen_logging_add_source_dir path)
    string(REGEX REPLACE "([\\\"])" "\\\\\\1" path_escaped "${path}")
    list(PREPEND oxen_logging_source_roots "\"${path_escaped}\"")
    list(REMOVE_DUPLICATES oxen_logging_source_roots)
    set(oxen_logging_source_roots "${oxen_logging_source_roots}" CACHE INTERNAL "" FORCE)

    foreach(prop_c_d COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS)
        get_target_property(roots oxen-logging ${prop_c_d})
        list(FILTER roots EXCLUDE REGEX "^OXEN_LOGGING_SOURCE_ROOTS")
        set_target_properties(oxen-logging PROPERTIES ${prop_c_d} "${root}")
    endforeach()
    list(LENGTH oxen_logging_source_roots pathlen)
    list(JOIN oxen_logging_source_roots ", " paths)
    target_compile_definitions(oxen-logging PUBLIC
        OXEN_LOGGING_SOURCE_ROOTS=${paths}
        OXEN_LOGGING_SOURCE_ROOTS_LEN=${pathlen}
    )
    message(STATUS "Stripping source root ${path} from oxen-logging paths")
endfunction()

if (OXEN_LOGGING_RELEASE_TRACE)
    target_compile_definitions(oxen-logging PUBLIC OXEN_LOGGING_RELEASE_TRACE)
endif()

add_library(oxen::logging ALIAS oxen-logging)
