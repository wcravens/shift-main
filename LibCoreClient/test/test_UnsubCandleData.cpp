#define BOOST_TEST_MODULE test_UnsubCandleData
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

#include <iostream>
#include <string>
#include <type_traits>

BOOST_AUTO_TEST_CASE(UNSUBCANDLEDATATEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    const std::string stockName = testClient->getStockList()[0];

    testClient->subCandleData(stockName);
    sleep(5);

    // get previous size of subscribed candle data list
    auto prev_size = testClient->getSubscribedCandlestickList().size();

    testClient->unsubCandleData(stockName);
    sleep(5);

    // get after size of subscribed candle data list
    auto after_size = testClient->getSubscribedCandlestickList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size - 1);
}
