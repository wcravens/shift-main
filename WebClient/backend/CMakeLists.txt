### CMake Version ##############################################################

cmake_minimum_required(VERSION 3.10)

### Project Configuration ######################################################

project(WebClient
        VERSION 1.0.0.0 # <major>.<minor>.<patch>.<tweak>
        LANGUAGES CXX)

### List of Files ##############################################################

set(CMAKE
    ${PROJECT_SOURCE_DIR}/cmake/FindLibuuid.cmake
)

set(INCLUDE
    ${PROJECT_SOURCE_DIR}/include/DBConnector.h
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
    ${PROJECT_SOURCE_DIR}/service/thrift/shiftpy/shift_service/__init__.py
    ${PROJECT_SOURCE_DIR}/service/thrift/shiftpy/shift_service/constants.py
    ${PROJECT_SOURCE_DIR}/service/thrift/shiftpy/shift_service/SHIFTService-remote
    ${PROJECT_SOURCE_DIR}/service/thrift/shiftpy/shift_service/SHIFTService.py
    ${PROJECT_SOURCE_DIR}/service/thrift/shiftpy/shift_service/ttypes.py
    ${PROJECT_SOURCE_DIR}/service/thrift/shiftpy/__init__.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/client.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testChangePW.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testGetByDay.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testGetLeaderboard.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testGetUser.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testIsLogin.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testLogin.py
    ${PROJECT_SOURCE_DIR}/service/thrift/tests/testRegister.py
    ${PROJECT_SOURCE_DIR}/service/thrift/py-gen.sh
    ${PROJECT_SOURCE_DIR}/service/thrift/shift_service.thrift
)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/DBConnector.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/MainClient.cpp
    ${PROJECT_SOURCE_DIR}/src/MyZMQ.cpp
    ${PROJECT_SOURCE_DIR}/src/SHIFTServiceHandler.cpp
    ${PROJECT_SOURCE_DIR}/src/UserClient.cpp
)

### Compiler Flags #############################################################

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions("-DHAVE_STD_UNIQUE_PTR=1") # QuickFIX C++17 compatibility req.

# Common Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fexceptions")

# Debug Flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

# Release Flags
# -O2 instead of -O3
# -ftlo stands for Link Time Optimization (LTO)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -finline-functions -flto")

# GCC (Ubuntu 20.04 LTS Focal Fossa)
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    include_directories(/usr/include/postgresql)
endif(UNIX AND NOT APPLE)

# GCC (Raspberry Pi OS)
# Ubuntu rules apply here as well
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv7l")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv7l")

# Clang (macOS 11 Big Sur)
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
    include_directories(/usr/local/include)
endif(APPLE)

### Build Types ################################################################

# If no build type is set, the default is Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

### Build Configuration ########################################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

########################################
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
########################################

find_package(Boost REQUIRED
             COMPONENTS program_options)

find_package(nlohmann_json 3.2.0 REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(PostgreSQL REQUIRED)

find_package(Threads REQUIRED)

find_library(ZEROMQ zmq)
if(NOT ZEROMQ)
    message(FATAL_ERROR "ZeroMQ was not found.")
endif(NOT ZEROMQ)

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

add_executable(${PROJECT_NAME}
               ${INCLUDE} ${SERVICE} ${SRC} ${OTHER})

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_PREFIX_PATH}/include
                           PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
                      ${THRIFT_LIBRARIES}
                      ${Boost_LIBRARIES}
                      nlohmann_json::nlohmann_json
                      ${OPENSSL_LIBRARIES}
                      ${PostgreSQL_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${ZEROMQ}
                      ${LIBCORECLIENT}
                      ${LIBMISCUTILS})

# Required when linking <filesystem> using GCC < 9.1
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif(UNIX AND NOT APPLE)

if(APPLE)
    find_library(CFLIB CoreFoundation)
    target_link_libraries(${PROJECT_NAME} ${CFLIB})
    target_compile_definitions(${PROJECT_NAME} PRIVATE __APPLE__)
else()
    find_package(Libuuid REQUIRED)
    if (NOT LIBUUID_FOUND)
        message(FATAL_ERROR "You might need to run 'sudo apt-get install uuid-dev' or similar")
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUUID_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${LIBUUID_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE __linux__)
endif()

### Install Configuration ######################################################

# If no installation path is set, the default is /usr/local
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif(NOT CMAKE_INSTALL_PREFIX)

# Install only if build type is set to Release
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        CONFIGURATIONS Release)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/../config/
        DESTINATION share/shift/${PROJECT_NAME}
        CONFIGURATIONS Release)

################################################################################
