#define BOOST_TEST_MODULE test_SubOrderBook
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(SUBORDERBOOKTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];

    // get previous size of subscribed order book data list
    int prevSize = testClient.getSubscribedOrderBookList().size();

    testClient.subOrderBook(stockName);
    sleep(5);

    // get after size of subscribed order book data list
    int afterSize = testClient.getSubscribedOrderBookList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize + 1);
}
