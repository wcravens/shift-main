#define BOOST_TEST_MODULE test_MarketBuy
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(MARKETBUYTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    const std::string stockName = testClient.getStockList()[0];
    testClient.subOrderBook(stockName);
    sleep(5);

    int prevSize = testClient.getPortfolioSummary().getTotalShares();
    std::cout << "previous size should be: " << prevSize << std::endl;

    Order marketBuy(shift::Order::Type::MARKET_BUY, stockName, TESTSIZE);
    testClient.submitOrder(marketBuy);
    sleep(5);

    auto afterSize = testClient.getPortfolioSummary().getTotalShares();
    std::cout << "after size should be: " << afterSize << std::endl;

    // test multiple clients working in multiple threads
    std::vector<std::unique_ptr<CoreClient>> clients;
    for (int i = 0; i < 9; ++i) {
        clients.push_back(std::make_unique<CoreClient>("test00" + std::to_string(i + 1))); // pointer clients
        initiator.attachClient(clients[i].get());
    }
    sleep(5);

    std::vector<int> beforeShares;
    std::vector<std::unique_ptr<std::thread>> threads;
    for (int i = 0; i < 9; ++i) {
        beforeShares.push_back(clients[i]->getPortfolioSummary().getTotalShares());

        // this part should not be a problem, since FIX::Session::sendToTarget(message) is already thread-safe
        threads.push_back(std::make_unique<std::thread>(&CoreClient::submitOrder, clients[i].get(), marketBuy));

        sleep(1);
    }
    // sleep(5);

    for (auto& pt : threads) {
        pt->join();
    }

    int i = 0;
    for (auto& pc : clients) {
        BOOST_CHECK_EQUAL(pc->getPortfolioSummary().getTotalShares(), beforeShares[i++] + TESTSIZE * 100);
    }

    testClient.unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(afterSize, prevSize + TESTSIZE * 100);
}
