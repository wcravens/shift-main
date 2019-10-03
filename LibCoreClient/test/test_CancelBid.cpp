#define BOOST_TEST_MODULE test_CancelBid
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(CANCELBIDTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    const std::string stockName = testClient->getStockList()[0];
    testClient->subOrderBook(stockName);
    sleep(5);

    BestPrice bestPrice = testClient->getBestPriceBySymbol(stockName);
    double limitBuyPrice = bestPrice.getBidPrice() - 1;
    std::cout << "limitBuyPrice: " << limitBuyPrice << std::endl;

    Order limitbuy(stockName, limitBuyPrice, TESTSIZE, shift::Order::Type::LIMIT_BUY);
    testClient->submitOrder(limitbuy);
    sleep(5);

    auto prev_size = testClient->getWaitingListSize();
    std::cout << "previous size should be: " << prev_size << std::endl;

    const std::string orderID = testClient->getWaitingList()[0].getID();
    Order cancelBid(stockName, limitBuyPrice, TESTSIZE, shift::Order::Type::CANCEL_BID, orderID);
    testClient->submitOrder(cancelBid);
    sleep(5);

    auto after_size = testClient->getWaitingListSize();
    std::cout << "after size should be: " << after_size << std::endl;

    testClient->unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size - TESTSIZE);
}