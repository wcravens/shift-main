#define BOOST_TEST_MODULE test_SubCandlestickData
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(SUBCANDLESTICKDATATEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];

    // get previous size of subscribed candlestick data list
    int prevSize = testClient.getSubscribedCandlestickDataList().size();

    testClient.subCandlestickData(stockName);
    sleep(5);

    // get after size of subscribed candlestick data list
    int afterSize = testClient.getSubscribedCandlestickDataList().size();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize + 1);
}
