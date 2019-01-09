### CMake Version #############################################################

cmake_minimum_required(VERSION 3.10)

### Project Configuration #####################################################

get_filename_component(PROJECT_DIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" PROJECT_DIR_NAME ${PROJECT_DIR_NAME})

project(${PROJECT_DIR_NAME}
        VERSION 1.0.0.0 # <major>.<minor>.<patch>.<tweak>
        LANGUAGES CXX)

### List of Files #############################################################

IF(EXISTS ${PROJECT_SOURCE_DIR}/config)
    set(CONFIG
        ${PROJECT_SOURCE_DIR}/config/acceptor.cfg
        ${PROJECT_SOURCE_DIR}/config/config.txt
        ${PROJECT_SOURCE_DIR}/config/initiator.cfg
    )
endif(EXISTS ${PROJECT_SOURCE_DIR}/config)

set(CONFIGDEFAULT
    ${PROJECT_SOURCE_DIR}/config.default/acceptor.cfg
    ${PROJECT_SOURCE_DIR}/config.default/config.txt
    ${PROJECT_SOURCE_DIR}/config.default/initiator.cfg
)

set(INCLUDE
    ${PROJECT_SOURCE_DIR}/include/CommonStock.h
    ${PROJECT_SOURCE_DIR}/include/Instrument.h
    ${PROJECT_SOURCE_DIR}/include/LimitOrder.h
    ${PROJECT_SOURCE_DIR}/include/MarketOrder.h
    ${PROJECT_SOURCE_DIR}/include/Order.h
    ${PROJECT_SOURCE_DIR}/include/PriceLevel.h
)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/CommonStock.cpp
    ${PROJECT_SOURCE_DIR}/src/Instrument.cpp
    ${PROJECT_SOURCE_DIR}/src/LimitOrder.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/MarketOrder.cpp
    ${PROJECT_SOURCE_DIR}/src/Order.cpp
    ${PROJECT_SOURCE_DIR}/src/PriceLevel.cpp
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
# -Wno-deprecated is required when linking QuickFIX on Ubuntu
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
endif(UNIX AND NOT APPLE)

# Clang (macOS Mojave 10.14)
# -Wno-overloaded-virtual is required when linking QuickFIX on macOS
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
endif(APPLE)

### Build Types ###############################################################

# if no build type is set, the default is Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

### Build Configuration #######################################################

find_package(Boost REQUIRED
             COMPONENTS date_time program_options)

find_package(Threads REQUIRED)

find_library(QUICKFIX quickfix)
if(NOT QUICKFIX)
    message(FATAL_ERROR "QuickFIX was not found.")
endif(NOT QUICKFIX)

add_executable(${PROJECT_NAME}
               ${CONFIG} ${CONFIGDEFAULT} ${INCLUDE} ${SRC} ${OTHER})

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
                      ${Boost_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${QUICKFIX})

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
install(DIRECTORY ${PROJECT_SOURCE_DIR}/config/
        DESTINATION share/shift/${PROJECT_NAME}
        CONFIGURATIONS Release)

###############################################################################
