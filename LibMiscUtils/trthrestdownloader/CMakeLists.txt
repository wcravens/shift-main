### CMake Version #############################################################

cmake_minimum_required(VERSION 3.10)

### Compiler Flags ############################################################

# Clang (macOS Mojave 10.14)
if(APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    include_directories(/usr/local/opt/openssl/include)
endif(APPLE)

### Build Types ###############################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

### Build Configuration #######################################################

find_package(Boost REQUIRED
             COMPONENTS program_options thread)

find_package(OpenSSL REQUIRED)

#find_package(Threads REQUIRED)

find_package(ZLIB REQUIRED)

find_library(CPPREST cpprest)
if(NOT CPPREST)
    message(FATAL_ERROR "CPPREST was not found.")
endif(NOT CPPREST)

add_executable(TRTHRESTDownloader
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/tools/comp_cols.py
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/tools/read_cols_2.py
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/tools/read_cols.py
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/cred.json
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/extract_raw.json
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/main.cpp
               ${PROJECT_SOURCE_DIR}/trthrestdownloader/README.txt)

target_link_libraries(TRTHRESTDownloader
                      shift_${LIB_NAME}
                      ${Boost_LIBRARIES}
                      ${OPENSSL_LIBRARIES}
                      #${CMAKE_THREAD_LIBS_INIT}
                      ${ZLIB_LIBRARIES}
                      ${CPPREST})

# cred.json
configure_file(${PROJECT_SOURCE_DIR}/trthrestdownloader/cred.json
               ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/cred.json
               COPYONLY)

# extract_raw.json
configure_file(${PROJECT_SOURCE_DIR}/trthrestdownloader/extract_raw.json
               ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/extract_raw.json
               COPYONLY)

###############################################################################
