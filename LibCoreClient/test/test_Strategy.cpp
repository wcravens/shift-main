#define BOOST_TEST_MODULE test_Strategy
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

#include "strategies/StrategyFactory.h"
#include "strategies/StrategyParameter.h"

BOOST_AUTO_TEST_CASE(STRATEGYTEST) //strategy is no long used right?
{
    auto& initiator = shift::FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password", 1000);

    // shift::strategies::StrategyParameter sp{ false };
    // sp = 3.4;
    // std::cout << "double? " << (sp.isDouble() ? "true" : "false") << "\n";
    // std::cout << "value: " << sp << "\n";
    // sp = true;
    // std::cout << "bool? " << (sp.isBool() ? "true" : "false") << "\n";
    // std::cout << "value: " << sp << "\n";
    // sp = !sp;
    // std::cout << "bool? " << (sp.isBool() ? "true" : "false") << "\n";
    // std::cout << "value: " << sp << "\n";
    // sp = "Test";
    // std::cout << "std::string? " << (sp.isString() ? "true" : "false") << "\n";
    // std::cout << "value: " << sp << "\n";

    auto strategy = shift::strategies::StrategyFactory::Instance().Create("ZeroIntelligence", *testClient, false, { 380, 190, 99.95, 100.05, 0.05 });
    strategy->run();

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_EQUAL(1, 1);
}
