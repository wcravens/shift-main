#define BOOST_TEST_MODULE test_connect
#define BOOST_TEST_DYN_LINK

#include "testUtils.h"

BOOST_AUTO_TEST_CASE(CONNECTTEST)
{
    auto& initiator = FIXInitiator::getInstance();

    CoreClient* testClient = new CoreClient("test010");
    bool logonResult = initiator.connectBrokerageCenter("initiator.cfg", testClient, "password");

    initiator.disconnectBrokerageCenter();
    BOOST_CHECK(logonResult);
}
