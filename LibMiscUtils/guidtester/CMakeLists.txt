### CMake Version ##############################################################

cmake_minimum_required(VERSION 3.10)

### Build Types ################################################################

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

### Build Configuration ########################################################

add_executable(GUIDTester
               ${PROJECT_SOURCE_DIR}/guidtester/main.cpp)

target_link_libraries(GUIDTester
                      shift_${LIB_NAME})

################################################################################
