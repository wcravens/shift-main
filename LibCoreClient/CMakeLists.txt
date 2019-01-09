### CMake Version #############################################################

cmake_minimum_required(VERSION 3.10)

### Project Configuration #####################################################

get_filename_component(PROJECT_DIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" PROJECT_DIR_NAME ${PROJECT_DIR_NAME})

project(${PROJECT_DIR_NAME}
        VERSION 1.0.0.0 # <major>.<minor>.<patch>.<tweak>
        LANGUAGES CXX)

string(SUBSTRING ${PROJECT_NAME} 3 -1 LIB_NAME)
string(TOLOWER ${LIB_NAME} LIB_NAME)

### List of Files #############################################################

set(INCLUDE
    ${PROJECT_SOURCE_DIR}/include/strategies/IStrategy.h
    ${PROJECT_SOURCE_DIR}/include/strategies/IStrategyMaker.h
    ${PROJECT_SOURCE_DIR}/include/strategies/StrategyFactory.h
    ${PROJECT_SOURCE_DIR}/include/strategies/StrategyMaker.h
    ${PROJECT_SOURCE_DIR}/include/strategies/StrategyParameter.h
    ${PROJECT_SOURCE_DIR}/include/strategies/ZeroIntelligence.h
    ${PROJECT_SOURCE_DIR}/include/BestPrice.h
    ${PROJECT_SOURCE_DIR}/include/CoreClient_EXPORTS.h
    ${PROJECT_SOURCE_DIR}/include/CoreClient.h
    ${PROJECT_SOURCE_DIR}/include/Exceptions.h
    ${PROJECT_SOURCE_DIR}/include/FIXInitiator.h
    ${PROJECT_SOURCE_DIR}/include/Order.h
    ${PROJECT_SOURCE_DIR}/include/OrderBook.h
    ${PROJECT_SOURCE_DIR}/include/OrderBookEntry.h
    ${PROJECT_SOURCE_DIR}/include/OrderBookGlobalAsk.h
    ${PROJECT_SOURCE_DIR}/include/OrderBookGlobalBid.h
	${PROJECT_SOURCE_DIR}/include/OrderBookLocalAsk.h
    ${PROJECT_SOURCE_DIR}/include/OrderBookLocalBid.h
    ${PROJECT_SOURCE_DIR}/include/PortfolioItem.h
    ${PROJECT_SOURCE_DIR}/include/PortfolioSummary.h
    # ${PROJECT_SOURCE_DIR}/include/Report.h
)

set(PKGCONFIG
    ${PROJECT_SOURCE_DIR}/pkg-config/libshift_coreclient-d.pc
    ${PROJECT_SOURCE_DIR}/pkg-config/libshift_coreclient.pc
)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/strategies/StrategyFactory.cpp
    ${PROJECT_SOURCE_DIR}/src/strategies/StrategyParameter.cpp
    ${PROJECT_SOURCE_DIR}/src/strategies/ZeroIntelligence.cpp
    ${PROJECT_SOURCE_DIR}/src/BestPrice.cpp
    ${PROJECT_SOURCE_DIR}/src/CoreClient.cpp
    ${PROJECT_SOURCE_DIR}/src/FIXInitiator.cpp
    ${PROJECT_SOURCE_DIR}/src/Order.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBook.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBookEntry.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBookGlobalAsk.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBookGlobalBid.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBookLocalAsk.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBookLocalBid.cpp
    ${PROJECT_SOURCE_DIR}/src/PortfolioItem.cpp
    ${PROJECT_SOURCE_DIR}/src/PortfolioSummary.cpp
    # ${PROJECT_SOURCE_DIR}/src/Report.cpp
)

set(OTHER
    ${PROJECT_SOURCE_DIR}/win-build.sh
)

### Compiler Flags ############################################################

# Important for libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# UNIX only
if(NOT WIN32)

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

endif(NOT WIN32)

# Visual C++ (Visual Studio 2017)
if(WIN32)
    add_definitions(-DCURL_STATICLIB)
    include_directories(C:/quickfix/include)
    include_directories(${PROJECT_SOURCE_DIR}/../LibMiscUtils/include)
endif(WIN32)

### Build Types ###############################################################

# If no build type is set, the default is Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_DEBUG_POSTFIX -d)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

### Build Configuration #######################################################

if(NOT WIN32) # UNIX
    find_package(CURL REQUIRED)
    find_library(QUICKFIX quickfix)
else(WIN32)   # Windows
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        find_library(QUICKFIX quickfix
                    PATHS C:/quickfix/x64/Debug)
        set(CURL_INCLUDE_DIR "C:/curl-7.58.0/builds/libcurl-vc14-x64-debug-static-ipv6-sspi-winssl/include")
        set(CURL_LIBRARY "C:/curl-7.58.0/builds/libcurl-vc14-x64-debug-static-ipv6-sspi-winssl/lib/libcurl_a_debug.lib")
    endif(CMAKE_BUILD_TYPE MATCHES Debug)
    if(CMAKE_BUILD_TYPE MATCHES Release)
        find_library(QUICKFIX quickfix
                    PATHS C:/quickfix/x64/Release)
        set(CURL_INCLUDE_DIR "C:/curl-7.58.0/builds/libcurl-vc14-x64-release-static-ipv6-sspi-winssl/include")
        set(CURL_LIBRARY "C:/curl-7.58.0/builds/libcurl-vc14-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib") 
    endif(CMAKE_BUILD_TYPE MATCHES Release)

    find_package(CURL REQUIRED)
    if(CURL_FOUND)
        INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})  
        find_library(CURL curl)
    else(CURL_FOUND)
        message(FATAL_ERROR "Could not find the CURL library")
    endif(CURL_FOUND)
endif(NOT WIN32)

if(NOT QUICKFIX)
    message(FATAL_ERROR "QuickFIX was not found.")
endif(NOT QUICKFIX)

# Always enforce search for SHIFT LibMiscUtils
# (necessary when using the same name for Debug and Release)
set (LIBMISCUTILS "LIBMISCUTILS-NOTFOUND")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    find_library(LIBMISCUTILS shift_miscutils-d
                 PATHS ${PROJECT_SOURCE_DIR}/../LibMiscUtils/build/Debug)
endif(CMAKE_BUILD_TYPE MATCHES Debug)
if(CMAKE_BUILD_TYPE MATCHES Release)
    find_library(LIBMISCUTILS shift_miscutils
                 PATHS ${PROJECT_SOURCE_DIR}/../LibMiscUtils/build/Release)
endif(CMAKE_BUILD_TYPE MATCHES Release)
if(NOT LIBMISCUTILS)
    message(FATAL_ERROR "SHIFT LibMiscUtils was not found.")
endif(NOT LIBMISCUTILS)

if(NOT WIN32) # UNIX
    add_library(shift_${LIB_NAME} SHARED
                ${INCLUDE} ${PKGCONFIG} ${SRC} ${OTHER})
else(WIN32)   # Windows
    add_library(shift_${LIB_NAME} STATIC
                ${INCLUDE} ${PKGCONFIG} ${SRC} ${OTHER})
endif(NOT WIN32)

if(NOT WIN32) # UNIX
    target_link_libraries(shift_${LIB_NAME}
                          ${CURL_LIBRARIES}
                          ${QUICKFIX}
                          ${LIBMISCUTILS})
else(WIN32)   # Windows
    target_link_libraries(shift_${LIB_NAME}
                          ${CURL_LIBRARY}
                          ${QUICKFIX}
                          ${LIBMISCUTILS})
endif(NOT WIN32)

target_include_directories(shift_${LIB_NAME}
                           PUBLIC ${PROJECT_SOURCE_DIR}/include)

if(TESTING)
    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif(TESTING)

### Install Configuration #####################################################

# UNIX only
if(NOT WIN32)

# If no installation path is set, the default is /usr/local
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif(NOT CMAKE_INSTALL_PREFIX)

# Add installation path to list of include directories
# (required to use other project libraries during compilation)
include_directories(${CMAKE_INSTALL_PREFIX}/include)

# Install library and header files
install(TARGETS shift_${LIB_NAME}
        LIBRARY DESTINATION lib
        CONFIGURATIONS Debug Release)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION include/shift/coreclient
        CONFIGURATIONS Debug Release)

# Install pkg-config script
install(FILES pkg-config/libshift_coreclient.pc
        DESTINATION lib/pkgconfig)
install(FILES pkg-config/libshift_coreclient-d.pc
        DESTINATION lib/pkgconfig)

endif(NOT WIN32)

###############################################################################
