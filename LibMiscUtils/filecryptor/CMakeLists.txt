### CMake Version #############################################################

cmake_minimum_required(VERSION 3.10)

### Build Types ###############################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

### Build Configuration #######################################################

find_package(Boost REQUIRED
             COMPONENTS program_options)

add_executable(FileCryptor
               ${PROJECT_SOURCE_DIR}/filecryptor/main.cpp
               ${PROJECT_SOURCE_DIR}/filecryptor/sample.txt)

target_link_libraries(FileCryptor
                      shift_${LIB_NAME}
                      ${Boost_LIBRARIES})

###############################################################################
