#define BOOST_TEST_MODULE test_CreateSymbolMap
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(CREATESYMBOLMAPTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    bool logonResult = initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    auto stockList = testClient.getStockList();
    for (const auto& symbol : stockList) {
        std::cout << symbol << std::endl;
    }

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK(logonResult);
}
