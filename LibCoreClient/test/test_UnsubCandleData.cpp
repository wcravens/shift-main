#define BOOST_TEST_MODULE test_UnsubCandleData
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

#include <iostream>
#include <string>
#include <type_traits>

BOOST_AUTO_TEST_CASE(UNSUBCANDLEDATATEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];

    testClient.subCandleData(stockName);
    sleep(5);

    // get previous size of subscribed candlestick data list
    auto prevSize = testClient.getSubscribedCandlestickList().size();

    testClient.unsubCandleData(stockName);
    sleep(5);

    // get after size of subscribed candlestick data list
    auto afterSize = testClient.getSubscribedCandlestickList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize - 1);
}
