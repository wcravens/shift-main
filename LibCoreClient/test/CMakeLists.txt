### CMake Version #############################################################

cmake_minimum_required(VERSION 3.10)

### Project Configuration #####################################################

set(TEST_PROJECT_NAME "TestClient")

### List of Files #############################################################

set(TESTS
    # test_Connect
    # test_CompanyList
    # test_CreateSymbolMap

    # test_LimitBuy
    # test_CancelBid
    # test_LimitSell
    # test_CancelAsk
    # test_MarketBuy
    # test_MarketSell
    # test_SubCandleData
    # test_UnsubCandleData
    # test_SubOrderBook
    # test_UnsubOrderBook
)

set(OTHER
    ${PROJECT_SOURCE_DIR}/initiator.cfg
)

### Build Configuration #######################################################

find_package(Boost REQUIRED
             COMPONENTS unit_test_framework)

find_package(Threads REQUIRED)

foreach(T ${TESTS})
    add_executable(${T} ${T}.cpp)
    target_link_libraries(${T}
                          shift_${LIB_NAME}
                          ${Boost_LIBRARIES}
                          ${CMAKE_THREAD_LIBS_INIT})
    add_test(NAME ${T} COMMAND ${T})
endforeach(T ${TESTS})

configure_file(${PROJECT_SOURCE_DIR}/test/initiator.cfg
               ${PROJECT_BINARY_DIR}/test/initiator.cfg
               COPYONLY)

###############################################################################
