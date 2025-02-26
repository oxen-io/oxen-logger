cmake_minimum_required(VERSION 3.15...3.23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "macOS deployment target (Apple clang only)")

project(oxen-logging VERSION 1.1.0 LANGUAGES CXX)

option(OXEN_LOGGING_WARNINGS_AS_ERRORS "treat all warnings as errors. turn off for development, on for release" OFF)
option(OXEN_LOGGING_FORCE_SUBMODULES "Force use of the bundled fmt/spdlog rather than looking for system packages" OFF)
option(OXEN_LOGGING_RELEASE_TRACE "Enable trace logging in release builds" OFF)
option(OXEN_LOGGING_FMT_HEADER_ONLY "Use fmt in header-only mode" OFF)
option(OXEN_LOGGING_SPDLOG_HEADER_ONLY "Use spdlog in header-only mode" OFF)
option(BUILD_SHARED_LIBS "Build a shared library instead of static library" OFF)
option(OXEN_LOGGING_VERSION_SO "Add the project major/minor version into the shared library filename" OFF)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(oxen_logging_IS_TOPLEVEL_PROJECT TRUE)
else()
    set(oxen_logging_IS_TOPLEVEL_PROJECT FALSE)
endif()

if(BUILD_SHARED_LIBS OR oxen_logging_IS_TOPLEVEL_PROJECT)
    set(install_default ON)
else()
    set(install_default OFF)
endif()
option(OXEN_LOGGING_INSTALL "Install oxen-logging library and headers to cmake install target; defaults to ON if BUILD_SHARED_LIBS is enabled or when building as the top-level project" ${install_default})

if(MINGW OR ANDROID OR IOS)
  set(use_lto_default OFF)
else()
  set(use_lto_default ON)
endif()
option(USE_LTO "Use Link-Time Optimization" ${use_lto_default})

if(USE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_ENABLED OUTPUT ipo_error)
  if(IPO_ENABLED)
    message(STATUS "LTO enabled")
  else()
    message(WARNING "LTO not supported by compiler: ${ipo_error}")
  endif()
else()
  message(STATUS "LTO disabled")
  set(IPO_ENABLED OFF)
endif()

if(IPO_ENABLED AND NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Always build PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

add_library(oxen-logging
    src/catlogger.cpp
    src/level.cpp
    src/log.cpp
    src/type.cpp
)

if(OXEN_LOGGING_VERSION_SO)
    set_target_properties(oxen-logging PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
endif()

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

# This used to add a path prefix to be stripped out of log statements, but we now just strip all
# paths (only keeping the path basename) because the prefix stripping could cause ODR violations
# when linking multiple oxen-logging-using libraries together, and didn't seem worth fixing via
# runtime configuration versus just ignoring all directories.
function(oxen_logging_add_source_dir path)
    message(WARNING "oxen_logging_add_source_dir is deprecated and does nothing: oxen-logging now strips away all directory names")
endfunction()

if (OXEN_LOGGING_RELEASE_TRACE)
    target_compile_definitions(oxen-logging PUBLIC OXEN_LOGGING_RELEASE_TRACE)
endif()

add_library(oxen::logging ALIAS oxen-logging)

if(OXEN_LOGGING_INSTALL)
    include(GNUInstallDirs)

    install(
        TARGETS oxen-logging
        EXPORT oxenloggingConfig
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(DIRECTORY include/oxen DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} PATTERN *.hpp)

    configure_file(liboxen-logging.pc.in liboxen-logging.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/liboxen-logging.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )

endif()
