### CMake Version #############################################################

cmake_minimum_required(VERSION 3.10)

### Project Configuration #####################################################

project(WebClient
        VERSION 1.0.0.0 # <major>.<minor>.<patch>.<tweak>
        LANGUAGES CXX)

include_directories(${PROJECT_SOURCE_DIR}/include)

### List of Files #############################################################

set(INCLUDE
    ${PROJECT_SOURCE_DIR}/include/MainClient.h
    ${PROJECT_SOURCE_DIR}/include/MyZMQ.h
    ${PROJECT_SOURCE_DIR}/include/SHIFTServiceHandler.h
    ${PROJECT_SOURCE_DIR}/include/UserClient.h
)

set(SERVICE
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/shift_service_constants.cpp
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/shift_service_constants.h
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/shift_service_types.cpp
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/shift_service_types.h
    # ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/SHIFTService_server.skeleton.cpp
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/SHIFTService.cpp
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-cpp/SHIFTService.h
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-php/client/SHIFTService.php
    ${PROJECT_SOURCE_DIR}/service/thrift/gen-php/client/Types.php
    ${PROJECT_SOURCE_DIR}/service/thrift/shift_service.thrift
)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/MainClient.cpp
    ${PROJECT_SOURCE_DIR}/src/MyZMQ.cpp
    ${PROJECT_SOURCE_DIR}/src/SHIFTServiceHandler.cpp
    ${PROJECT_SOURCE_DIR}/src/UserClient.cpp
)

set(OTHER
    ${PROJECT_SOURCE_DIR}/debug.sh
)

### Compiler Flags ############################################################

# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -pedantic-errors")

# Debug Flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

# Release Flags
# -O2 instead of -O3
# -ftlo stands for Link Time Optimization (LTO)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -flto")

# GCC (Ubuntu 18.04 LTS Bionic Beaver)
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif(UNIX AND NOT APPLE)

# Clang (macOS Mojave 10.14)
# -Wno-overloaded-virtual is required when linking QuickFIX on macOS
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
    include_directories(/usr/local/include)
endif(APPLE)

### Build Types ###############################################################

# If no build type is set, the default is Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

### Build Configuration #######################################################

#######################################
# - Find Thrift (a cross platform RPC lib/tool)
# This module defines
#  THRIFT_VERSION_STRING, version string of ant if found
#  THRIFT_LIBRARIES, libraries to link
#  THRIFT_INCLUDE_DIR, where to find THRIFT headers
#  THRIFT_COMPILER, thrift compiler executable
#  THRIFT_FOUND, If false, do not try to use ant
# Function
#  thrift_gen_cpp(<path to thrift file> <output variable with file list>)
#
# Initial work was done by Cloudera https://github.com/cloudera/Impala
# 2014 - modified by snikulov

# Prefer the thrift version supplied in THRIFT_HOME (cmake -DTHRIFT_HOME then environment)
find_path(THRIFT_INCLUDE_DIR
    NAMES
        thrift/Thrift.h
    HINTS
        ${THRIFT_HOME}
        ENV THRIFT_HOME
        /usr/local
        /opt/local
    PATH_SUFFIXES
        include
)

# Prefer the thrift version supplied in THRIFT_HOME
find_library(THRIFT_LIBRARIES
    NAMES
        thrift libthrift
    HINTS
        ${THRIFT_HOME}
        ENV THRIFT_HOME
        /usr/local
        /opt/local
    PATH_SUFFIXES
        lib lib64
)

find_program(THRIFT_COMPILER
    NAMES
        thrift
    HINTS
        ${THRIFT_HOME}
        ENV THRIFT_HOME
        /usr/local
        /opt/local
    PATH_SUFFIXES
        bin bin64
)

if (THRIFT_COMPILER)
    exec_program(${THRIFT_COMPILER}
        ARGS -version OUTPUT_VARIABLE __thrift_OUT RETURN_VALUE THRIFT_RETURN)
    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+-[a-z]+$" THRIFT_VERSION_STRING ${__thrift_OUT})

    # define utility function to generate cpp files
    function(thrift_gen_cpp thrift_file THRIFT_CPP_FILES_LIST THRIFT_GEN_INCLUDE_DIR)
        set(_res)
        set(_res_inc_path)
        if(EXISTS ${thrift_file})
            get_filename_component(_target_dir ${thrift_file} NAME_WE)
            message("thrif_gen_cpp: ${thrift_file}")

            if(NOT EXISTS ${CMAKE_BINARY_DIR}/${_target_dir})
                file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${_target_dir})
            endif()
            exec_program(${THRIFT_COMPILER}
                ARGS -o "${CMAKE_BINARY_DIR}/${_target_dir}" --gen cpp ${thrift_file}
                OUTPUT_VARIABLE __thrift_OUT
                RETURN_VALUE THRIFT_RETURN)
            file(GLOB_RECURSE __result_src "${CMAKE_BINARY_DIR}/${_target_dir}/*.cpp")
            file(GLOB_RECURSE __result_hdr "${CMAKE_BINARY_DIR}/${_target_dir}/*.h")
            list(APPEND _res ${__result_src})
            list(APPEND _res ${__result_hdr})
            if(__result_hdr)
                list(GET __result_hdr 0 _res_inc_path)
                get_filename_component(_res_inc_path ${_res_inc_path} DIRECTORY)
            endif()
        else()
            message("thrift_gen_cpp: file ${thrift_file} does not exists")
        endif()
        set(${THRIFT_CPP_FILES_LIST} "${_res}" PARENT_SCOPE)
        set(${THRIFT_GEN_INCLUDE_DIR} "${_res_inc_path}" PARENT_SCOPE)
    endfunction()
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(THRIFT DEFAULT_MSG THRIFT_LIBRARIES THRIFT_INCLUDE_DIR THRIFT_COMPILER)
mark_as_advanced(THRIFT_LIBRARIES THRIFT_INCLUDE_DIR THRIFT_COMPILER THRIFT_VERSION_STRING)

#######################################

find_package(Boost REQUIRED
             COMPONENTS program_options)

find_package(cppzmq REQUIRED)

find_package(Threads REQUIRED)

find_library(QUICKFIX quickfix)
if(NOT QUICKFIX)
    message(FATAL_ERROR "QuickFIX was not found.")
endif(NOT QUICKFIX)

# Always enforce search for SHIFT LibMiscUtils
# (necessary when using the same name for Debug and Release)
set (LIBMISCUTILS "LIBMISCUTILS-NOTFOUND")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    find_library(LIBMISCUTILS shift_miscutils-d)
endif(CMAKE_BUILD_TYPE MATCHES Debug)
if(CMAKE_BUILD_TYPE MATCHES Release)
    find_library(LIBMISCUTILS shift_miscutils)
endif(CMAKE_BUILD_TYPE MATCHES Release)
if(NOT LIBMISCUTILS)
    message(FATAL_ERROR "SHIFT LibMiscUtils was not found.")
endif(NOT LIBMISCUTILS)

# Always enforce search for SHIFT LibCoreClient
# (necessary when using the same name for Debug and Release)
set (LIBCORECLIENT "LIBCORECLIENT-NOTFOUND")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    find_library(LIBCORECLIENT shift_coreclient-d)
endif(CMAKE_BUILD_TYPE MATCHES Debug)
if(CMAKE_BUILD_TYPE MATCHES Release)
    find_library(LIBCORECLIENT shift_coreclient)
endif(CMAKE_BUILD_TYPE MATCHES Release)
if(NOT LIBCORECLIENT)
    message(FATAL_ERROR "SHIFT LibCoreClient was not found.")
endif(NOT LIBCORECLIENT)

add_executable(${PROJECT_NAME}
               ${INCLUDE} ${SERVICE} ${SRC} ${OTHER})

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
                      ${THRIFT_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${cppzmq_LIBRARY}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${QUICKFIX}
                      ${LIBMISCUTILS}
                      ${LIBCORECLIENT})

### Install Configuration #####################################################

# If no installation path is set, the default is /usr/local
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif(NOT CMAKE_INSTALL_PREFIX)

# Add installation path to list of include directories
# (required to use other project libraries during compilation)
include_directories(${CMAKE_INSTALL_PREFIX}/include)

# Install only if build type is set to Release
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        CONFIGURATIONS Release)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/../config/
        DESTINATION share/shift/${PROJECT_NAME}
        CONFIGURATIONS Release)

###############################################################################
