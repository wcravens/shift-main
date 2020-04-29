#define BOOST_TEST_MODULE test_SubCandleData
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(SUBCANDLEDATATEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];

    // get previous size of subscribed candlestick data list
    int prevSize = testClient.getSubscribedCandlestickList().size();

    testClient.subCandleData(stockName);
    sleep(5);

    // get after size of subscribed candlestick data list
    int afterSize = testClient.getSubscribedCandlestickList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize + 1);
}
