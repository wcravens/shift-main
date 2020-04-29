#define BOOST_TEST_MODULE test_CompanyList
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(COMPANYLISTTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient testClient { "test010" };
    initiator.connectBrokerageCenter("initiator.cfg", &testClient, "password");

    int prevSize = testClient.getCompanyNames().size();
    std::cout << "before check the size: " << prevSize << std::endl;

    testClient.requestCompanyNames();

    int size = testClient.getStockList().size();
    while (testClient.getCompanyNames().size() != size) {
        sleep(1);
    }

    int afterSize = testClient.getCompanyNames().size();
    std::cout << "after check the size: " << afterSize << std::endl;

    for (const auto& [ticker, companyName] : testClient.getCompanyNames()) {
        std::cout << ticker << " - " << companyName << std::endl;
    }

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK_NE(afterSize, prevSize);
}
