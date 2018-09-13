#define BOOST_TEST_MODULE test_SubOrderBook
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(SUBORDERBOOKTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    const std::string stockName = testClient->getStockList()[0];

    // get previous size of subscribed orderbook data list
    auto prev_size = testClient->getSubscribedOrderBookList().size();

    testClient->subOrderBook(stockName);
    sleep(5);

    // get after size of subscribed orderbook data list
    auto after_size = testClient->getSubscribedOrderBookList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size + 1);
}
