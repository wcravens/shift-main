#define BOOST_TEST_MODULE test_MarketBuy
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(MARKETSELLTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    const std::string stockName = testClient->getStockList()[0];
    testClient->subOrderBook(stockName);
    sleep(5);

    BestPrice bestPrice = testClient->getBestPriceBySymbol(stockName);
    double bestAskPrice = bestPrice.getAskPrice();
    std::cout << "bestAskPrice: " << bestAskPrice << std::endl;

    auto prev_size = testClient->getPortfolioSummary().getTotalShares();
    std::cout << "previous size should be: " << prev_size << std::endl;

    Order order(stockName, 0.0, TESTSIZE, shift::Order::Type::MARKET_SELL);
    testClient->submitOrder(order);
    sleep(5);

    auto after_size = testClient->getPortfolioSummary().getTotalShares();
    std::cout << "after size should be: " << after_size << std::endl;

    testClient->unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size + TESTSIZE * 100);
}
