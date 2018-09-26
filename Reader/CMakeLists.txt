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
project("Keyboard Hook Reader" C CXX)
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

find_package(
  Boost 1.55
  COMPONENTS
  program_options
  system
  REQUIRED)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops")

file(GLOB_RECURSE CPP_FILES
  "${PROJECT_DIR}/source/*.cpp")

add_executable(KeyboardHookReader ${CPP_FILES})

target_include_directories(
  KeyboardHookReader
  SYSTEM PRIVATE
  ${Boost_INCLUDE_DIRS})


target_link_libraries(
  KeyboardHookReader
  ${Boost_LIBRARIES}
  evdev
  pthread)

install(TARGETS KeyboardHookReader
        ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        RUNTIME DESTINATION ${INSTALL_BINARY_DIR})
