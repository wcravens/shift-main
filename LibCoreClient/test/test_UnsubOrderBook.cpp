#define BOOST_TEST_MODULE test_UnsubOrderBook
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

#include <iostream>
#include <string>
#include <type_traits>

BOOST_AUTO_TEST_CASE(UNSUBORDERBOOKTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    const std::string stockName = testClient->getStockList()[0];

    testClient->subOrderBook(stockName);
    sleep(5);

    // get previous size of subscribed orderbook data list
    int prev_size = testClient->getSubscribedOrderBookList().size();

    testClient->unsubOrderBook(stockName);
    sleep(5);

    // get after size of subscribed orderbook data list
    int after_size = testClient->getSubscribedOrderBookList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size - 1);
}
