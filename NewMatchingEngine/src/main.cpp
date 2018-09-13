#include "CommonStock.h"
#include "LimitOrder.h"
#include "MarketOrder.h"

#include <map>
#include <memory>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <shift/miscutils/terminal/Common.h>

int main(int ac, char* av[])
{
    /* Order Class Test */
    // std::vector<std::unique_ptr<Order>> order_list;

    // for (int i = 0; i < 50; i++) {
    //     std::unique_ptr<Order> limit(new LimitOrder(false, "orderID", "brokerID", "traderID", "destination",
    //         std::unique_ptr<Instrument>(new CommonStock("AAPL")), Order::SIDE::BUY, i, boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()), i));

    //     order_list.push_back(std::move(limit));

    //     std::unique_ptr<Order> market(new MarketOrder(false, "orderID", "brokerID", "traderID", "destination",
    //         std::unique_ptr<Instrument>(new CommonStock("MSFT")), Order::SIDE::BUY, i, boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time())));

    //     order_list.push_back(std::move(market));
    // }

    /* Instrument static_cast Test */
    // for (auto& o : order_list) {

    //     cout << o->getInstrument()->getTypeString() << endl;
    //     if (o->getInstrument()->getType() == Instrument::TYPE::COMMON_STOCK) {
    //         auto s = static_cast<const CommonStock*>(o->getInstrument());
    //         cout << s->getTypeString() << endl;
    //     }
    // }

    /* Instrument Comparator Test */
    // std::unique_ptr<Instrument> aapl_1(new CommonStock("AAPL"));
    // std::unique_ptr<Instrument> aapl_2(new CommonStock("AAPL"));
    // std::unique_ptr<Instrument> aapl_3(new CommonStock("AAPL"));

    // cout << Instrument::Comparator()(aapl_1, aapl_2) << endl;

    // std::map<std::unique_ptr<Instrument>, int, Instrument::Comparator> test;

    // test.insert(std::pair<std::unique_ptr<Instrument>, int>(std::move(aapl_1), 100));
    // test.insert(std::pair<std::unique_ptr<Instrument>, int>(std::move(aapl_2), 200));

    // auto it = test.find(aapl_3);
    // cout << it->second << endl;

    /* Copy Constructor Test */
    LimitOrder one(false, "orderID", "brokerID", "traderID", "destination",
        std::unique_ptr<Instrument>(new CommonStock("AAPL")), Order::SIDE::BUY, 1, boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()), 100);

    LimitOrder two(one);

    LimitOrder three(false, "orderID", "brokerID", "traderID", "destination",
        std::unique_ptr<Instrument>(new CommonStock("MSFT")), Order::SIDE::BUY, 1, boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()), 100);

    three = two;

    /* Wait Keyboard Input */
    int t;
    cin >> t;

    return 0;
}
