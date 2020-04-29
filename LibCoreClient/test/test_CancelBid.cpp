#define BOOST_TEST_MODULE test_CancelBid
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(CANCELBIDTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];
    testClient.subOrderBook(stockName);
    sleep(5);

    BestPrice bestPrice = testClient.getBestPrice(stockName);
    double limitBuyPrice = bestPrice.getBidPrice() - 1.00;
    std::cout << "limitBuyPrice: " << limitBuyPrice << std::endl;

    Order limitBuy(shift::Order::Type::LIMIT_BUY, stockName, TESTSIZE, limitBuyPrice);
    testClient.submitOrder(limitBuy);
    sleep(5);

    int prevSize = testClient.getWaitingListSize();
    std::cout << "previous size should be: " << prevSize << std::endl;

    const std::string orderID = testClient.getWaitingList()[0].getID();
    Order cancelBid(shift::Order::Type::CANCEL_BID, stockName, TESTSIZE, limitBuyPrice, orderID);
    testClient.submitOrder(cancelBid);
    sleep(5);

    int afterSize = testClient.getWaitingListSize();
    std::cout << "after size should be: " << afterSize << std::endl;

    testClient.unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize - TESTSIZE);
}