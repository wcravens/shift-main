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

set(CMAKE
    ${PROJECT_SOURCE_DIR}/cmake/FindLibuuid.cmake
)

set(INCLUDE
    ${PROJECT_SOURCE_DIR}/include/clock/Timestamp.h
    ${PROJECT_SOURCE_DIR}/include/concurrency/Consumer.h
    ${PROJECT_SOURCE_DIR}/include/crossguid/Guid.h
    ${PROJECT_SOURCE_DIR}/include/crypto/Decryptor.h
    ${PROJECT_SOURCE_DIR}/include/crypto/Encryptor.h
    ${PROJECT_SOURCE_DIR}/include/database/Tables.h
    ${PROJECT_SOURCE_DIR}/include/database/Common.h
    ${PROJECT_SOURCE_DIR}/include/statistics/BasicStatistics.h
    ${PROJECT_SOURCE_DIR}/include/terminal/Common.h
    ${PROJECT_SOURCE_DIR}/include/terminal/Functions.h
    ${PROJECT_SOURCE_DIR}/include/terminal/Options.h
    ${PROJECT_SOURCE_DIR}/include/MiscUtils_EXPORTS.h
)

set(PKGCONFIG
    ${PROJECT_SOURCE_DIR}/pkg-config/libshift_miscutils-d.pc
    ${PROJECT_SOURCE_DIR}/pkg-config/libshift_miscutils.pc
)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/clock/Timestamp.cpp
    ${PROJECT_SOURCE_DIR}/src/crossguid/Guid.cpp
    ${PROJECT_SOURCE_DIR}/src/crypto/Cryptor.cpp
    ${PROJECT_SOURCE_DIR}/src/crypto/Cryptor.h
    ${PROJECT_SOURCE_DIR}/src/crypto/Decryptor.cpp
    ${PROJECT_SOURCE_DIR}/src/crypto/Encryptor.cpp
    ${PROJECT_SOURCE_DIR}/src/crypto/SHA1.h
    ${PROJECT_SOURCE_DIR}/src/crypto/SHA1.cpp
    ${PROJECT_SOURCE_DIR}/src/database/Tables.cpp
    ${PROJECT_SOURCE_DIR}/src/database/Common.cpp
    ${PROJECT_SOURCE_DIR}/src/terminal/Common.cpp
    ${PROJECT_SOURCE_DIR}/src/terminal/Options.cpp
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
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif(UNIX AND NOT APPLE)

# Clang (macOS Mojave 10.14)
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    include_directories(/usr/local/include)
endif(APPLE)

endif(NOT WIN32)

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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

if(NOT WIN32) # UNIX
    add_library(shift_${LIB_NAME} SHARED
                ${CMAKE} ${INCLUDE} ${PKGCONFIG} ${SRC} ${OTHER})
else(WIN32)   # Windows
    add_library(shift_${LIB_NAME} STATIC
                ${CMAKE} ${INCLUDE} ${PKGCONFIG} ${SRC} ${OTHER})
endif(NOT WIN32)

target_include_directories(shift_${LIB_NAME}
                           PUBLIC ${CMAKE_PREFIX_PATH}/include
                           PUBLIC ${PROJECT_SOURCE_DIR}/include)

if(WIN32)
    target_compile_definitions(shift_${LIB_NAME} PRIVATE _WIN32)
elseif(APPLE)
    find_library(CFLIB CoreFoundation)
    target_link_libraries(shift_${LIB_NAME} ${CFLIB})
    target_compile_definitions(shift_${LIB_NAME} PRIVATE __APPLE__)
else()
    find_package(Libuuid REQUIRED)
    if (NOT LIBUUID_FOUND)
        message(FATAL_ERROR "You might need to run 'sudo apt-get install uuid-dev' or similar")
    endif()
    target_include_directories(shift_${LIB_NAME} PRIVATE ${LIBUUID_INCLUDE_DIR})
    target_link_libraries(shift_${LIB_NAME} ${LIBUUID_LIBRARY})
    target_compile_definitions(shift_${LIB_NAME} PRIVATE __linux__)
endif()

find_package(PostgreSQL REQUIRED)
target_link_libraries(shift_${LIB_NAME}
                      ${PostgreSQL_LIBRARIES}
                      )

if(ADDONS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/filecryptor)
    add_subdirectory(${PROJECT_SOURCE_DIR}/guidtester)
    add_subdirectory(${PROJECT_SOURCE_DIR}/trthrestdownloader)
endif(ADDONS)

### Install Configuration #####################################################

# UNIX only
if(NOT WIN32)

# If no installation path is set, the default is /usr/local
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif(NOT CMAKE_INSTALL_PREFIX)

# Install library and header files
install(TARGETS shift_${LIB_NAME}
        LIBRARY DESTINATION lib
        CONFIGURATIONS Debug Release)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION include/shift/miscutils
        CONFIGURATIONS Debug Release)

# Install pkg-config script
install(FILES pkg-config/libshift_miscutils.pc
        DESTINATION lib/pkgconfig)
install(FILES pkg-config/libshift_miscutils-d.pc
        DESTINATION lib/pkgconfig)

endif(NOT WIN32)

###############################################################################
