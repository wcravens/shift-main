#define BOOST_TEST_MODULE test_CancelAsk
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(CANCELASKTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];
    testClient.subOrderBook(stockName);
    sleep(5);

    BestPrice bestPrice = testClient.getBestPrice(stockName);
    double limitSellPrice = bestPrice.getAskPrice() + 1.00;
    std::cout << "limitSellPrice: " << limitSellPrice << std::endl;

    Order limitSell(shift::Order::Type::LIMIT_SELL, stockName, TESTSIZE, limitSellPrice);
    testClient.submitOrder(limitSell);
    sleep(5);

    int prevSize = testClient.getWaitingListSize();
    std::cout << "previous size should be: " << prevSize << std::endl;

    const std::string orderID = testClient.getWaitingList()[0].getID();
    Order cancelAsk(shift::Order::Type::CANCEL_ASK, stockName, TESTSIZE, limitSellPrice, orderID);
    testClient.submitOrder(cancelAsk);
    sleep(5);

    int afterSize = testClient.getWaitingListSize();
    std::cout << "after size should be: " << afterSize << std::endl;

    testClient.unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize - TESTSIZE);
}
