#define BOOST_TEST_MODULE test_CompanyList
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(COMPANYLISTTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    initiator.connectBrokerageCenter("initiator.cfg", testClient, "password");

    auto prev_size = testClient->getCompanyNames().size();
    std::cout << "before check the size: " << prev_size << std::endl;

    testClient->requestCompanyNames();

    auto size = testClient->getStockList().size();
    while (testClient->getCompanyNames().size() != size)
        sleep(1);

    auto after_size = testClient->getCompanyNames().size();
    std::cout << "after check the size: " << after_size << std::endl;

    for (const auto& [ticker, companyName] : testClient->getCompanyNames()) {
        std::cout << ticker << " - " << companyName << std::endl;
    }

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_NE(after_size, prev_size);
}
