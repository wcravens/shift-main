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
        ${PROJECT_SOURCE_DIR}/config/dbLogin.txt
        ${PROJECT_SOURCE_DIR}/config/initiator.cfg
    )
endif(EXISTS ${PROJECT_SOURCE_DIR}/config)

set(CONFIGDEFAULT
    ${PROJECT_SOURCE_DIR}/config.default/acceptor.cfg
    ${PROJECT_SOURCE_DIR}/config.default/dbLogin.txt
    ${PROJECT_SOURCE_DIR}/config.default/initiator.cfg
)

set(INCLUDE
    ${PROJECT_SOURCE_DIR}/include/BCDocuments.h
    ${PROJECT_SOURCE_DIR}/include/DBConnector.h
    ${PROJECT_SOURCE_DIR}/include/FIXAcceptor.h
    ${PROJECT_SOURCE_DIR}/include/FIXInitiator.h
    ${PROJECT_SOURCE_DIR}/include/OrderBookEntry.h
    ${PROJECT_SOURCE_DIR}/include/PortfolioItem.h
    ${PROJECT_SOURCE_DIR}/include/PortfolioSummary.h
    ${PROJECT_SOURCE_DIR}/include/Quote.h
    ${PROJECT_SOURCE_DIR}/include/Report.h
    ${PROJECT_SOURCE_DIR}/include/RiskManagement.h
    ${PROJECT_SOURCE_DIR}/include/Stock.h
    ${PROJECT_SOURCE_DIR}/include/CandlestickData.h
    ${PROJECT_SOURCE_DIR}/include/TempCandlestickData.h
    ${PROJECT_SOURCE_DIR}/include/TokenPool.h
    ${PROJECT_SOURCE_DIR}/include/Transaction.h
)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/BCDocuments.cpp
    ${PROJECT_SOURCE_DIR}/src/DBConnector.cpp
    ${PROJECT_SOURCE_DIR}/src/FIXAcceptor.cpp
    ${PROJECT_SOURCE_DIR}/src/FIXInitiator.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/OrderBookEntry.cpp
    ${PROJECT_SOURCE_DIR}/src/PortfolioItem.cpp
    ${PROJECT_SOURCE_DIR}/src/PortfolioSummary.cpp
    ${PROJECT_SOURCE_DIR}/src/Quote.cpp
    ${PROJECT_SOURCE_DIR}/src/RiskManagement.cpp
    ${PROJECT_SOURCE_DIR}/src/Stock.cpp
    ${PROJECT_SOURCE_DIR}/src/CandlestickData.cpp
    ${PROJECT_SOURCE_DIR}/src/TempCandlestickData.cpp
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
# -flto stands for Link Time Optimization (LTO)
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -flto")
# BrokerageCenter does not work with optimization flags - we need to fix this:
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

# GCC (Ubuntu 18.04 LTS Bionic Beaver)
# -Wno-deprecated is required when linking QuickFIX on Ubuntu
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
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

find_package(Boost REQUIRED
             COMPONENTS program_options)

find_package(PostgreSQL REQUIRED)

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

add_executable(${PROJECT_NAME}
               ${CONFIG} ${CONFIGDEFAULT} ${INCLUDE} ${SRC} ${OTHER})

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_PREFIX_PATH}/include
                           PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
                      ${Boost_LIBRARIES}
                      ${PostgreSQL_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${QUICKFIX}
                      ${LIBMISCUTILS})

### Install Configuration #####################################################

# If no installation path is set, the default is /usr/local
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif(NOT CMAKE_INSTALL_PREFIX)

# Install only if build type is set to Release
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        CONFIGURATIONS Release)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/config/
        DESTINATION share/shift/${PROJECT_NAME}
        CONFIGURATIONS Release)

###############################################################################
