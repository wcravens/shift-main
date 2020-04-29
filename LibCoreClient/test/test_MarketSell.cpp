#define BOOST_TEST_MODULE test_MarketBuy
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(MARKETSELLTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];
    testClient.subOrderBook(stockName);
    sleep(5);

    BestPrice bestPrice = testClient.getBestPrice(stockName);
    double bestAskPrice = bestPrice.getAskPrice();
    std::cout << "bestAskPrice: " << bestAskPrice << std::endl;

    int prevSize = testClient.getPortfolioSummary().getTotalShares();
    std::cout << "previous size should be: " << prevSize << std::endl;

    Order marketSell(shift::Order::Type::MARKET_SELL, stockName, TESTSIZE);
    testClient.submitOrder(marketSell);
    sleep(5);

    int afterSize = testClient.getPortfolioSummary().getTotalShares();
    std::cout << "after size should be: " << afterSize << std::endl;

    testClient.unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize + TESTSIZE * 100);
}
