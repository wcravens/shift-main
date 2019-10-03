#define BOOST_TEST_MODULE test_LimitSell
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(LIMITSELLTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password");

    const std::string stockName = testClient->getStockList()[0];
    testClient->subOrderBook(stockName);
    sleep(5);

    BestPrice bestPrice = testClient->getBestPriceBySymbol(stockName);
    double limitSellPrice = bestPrice.getAskPrice() + 1;
    std::cout << "limitSellPrice: " << limitSellPrice << std::endl;

    auto prev_size = testClient->getWaitingListSize();
    std::cout << "previous size should be: " << prev_size << std::endl;

    Order order(stockName, limitSellPrice, TESTSIZE, shift::Order::Type::LIMIT_SELL);
    testClient->submitOrder(order);
    sleep(5);

    auto after_size = testClient->getWaitingListSize();
    std::cout << "after size should be: " << after_size << std::endl;

    testClient->unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size + TESTSIZE);
}