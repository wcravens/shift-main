#define BOOST_TEST_MODULE test_MarketBuy
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(MARKETBUYTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    const std::string stockName = testClient->getStockList()[0];
    testClient->subOrderBook(stockName);
    sleep(5);

    auto prev_size = testClient->getPortfolioSummary().getTotalShares();
    std::cout << "previous size should be: " << prev_size << std::endl;

    Order order(stockName, 0.0, TESTSIZE, shift::Order::MARKET_BUY);
    testClient->submitOrder(order);
    sleep(5);

    auto after_size = testClient->getPortfolioSummary().getTotalShares();
    std::cout << "after size should be: " << after_size << std::endl;

    // test multiple clients works in muntiple threads
    std::vector<CoreClient*> clients;
    for (int i = 0; i < 9; ++i) {

        std::string clientUsername = "test01" + std::to_string(i + 1);
        CoreClient* pc = new CoreClient(clientUsername); // pointer client
        FIXInitiator::getInstance().attach(pc);

        pc->subOrderBook(stockName);
        clients.push_back(pc);
    }
    sleep(5);

    std::vector<size_t> before_shares;
    std::vector<std::thread*> threads;
    for (int i = 0; i < 9; ++i) {

        before_shares.push_back(clients[i]->getPortfolioSummary().getTotalShares());

        // this part should not be a problem, since FIX::session::sendMessage is already thread safe
        std::thread* pt = new std::thread(&CoreClient::submitOrder, clients[i], order);
        threads.push_back(pt);
        sleep(1);
    }
    //sleep(5);

    for (auto& pt : threads) {
        pt->join();
    }

    int i = 0;
    for (auto& pc : clients) {
        BOOST_CHECK_EQUAL(pc->getPortfolioSummary().getTotalShares(), before_shares[i++] + TESTSIZE * 100);
        pc->unsubOrderBook(stockName);
    }

    testClient->unsubOrderBook(stockName);
    sleep(5);

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(after_size, prev_size + TESTSIZE * 100);
}
