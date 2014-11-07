# General {{{
# ==============================================================================
cmake_minimum_required(VERSION 2.8.8)
# ------------------------------------------------------------------------------
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif ()
# ------------------------------------------------------------------------------
project("X11Hook" C CXX)
# ------------------------------------------------------------------------------
set(CMAKE_ERROR_DEPRECATED ON)
# ------------------------------------------------------------------------------
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
# ------------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# ------------------------------------------------------------------------------
get_filename_component(PROJECT_DIR "." ABSOLUTE)
set(INSTALL_BINARY_DIR  bin)
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
# ==============================================================================
# }}} General

# Compilers {{{
# ==============================================================================
if (CMAKE_COMPILER_IS_GNUCXX)
  set(COMPILER_IS_G++ TRUE)
  set(LINKER_IS_LD    TRUE)
endif ()
# ==============================================================================
# }}} Compilers

# Flags {{{
# ==============================================================================
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (COMPILER_IS_G++)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops")
endif ()

file(GLOB_RECURSE CPP_FILES
  "${PROJECT_DIR}/source/*.cpp")


add_executable(LinuxHookTest ${CPP_FILES})

target_link_libraries(LinuxHookTest evdev pthread)

install(TARGETS LinuxHookTest
        ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        RUNTIME DESTINATION ${INSTALL_BINARY_DIR})
