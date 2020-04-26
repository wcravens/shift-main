#include "strategies/ZeroIntelligence.h"

#include "strategies/StrategyMaker.h"

#ifdef _WIN32
#include <terminal/Common.h>
#else
#include <shift/miscutils/terminal/Common.h>
#endif

namespace shift {
namespace strategies {

    // public:

    static StrategyMaker<ZeroIntelligence> maker("ZeroIntelligence");

    ZeroIntelligence::ZeroIntelligence(shift::CoreClient& client, bool verbose, int strategy_duration, int trading_rate, double initial_bid, double initial_ask, double minimum_dolar_change)
        : IStrategy(client, verbose)
        , m_strategy_duration(strategy_duration)
        , m_trading_rate(trading_rate)
        , m_initial_bid(initial_bid)
        , m_initial_ask(initial_ask)
        , m_minimum_dolar_change(minimum_dolar_change)
    {
    }

    ZeroIntelligence::ZeroIntelligence(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters)
        : IStrategy(client, verbose)
    {
        std::initializer_list<StrategyParameter>::iterator it = parameters.begin();
        m_strategy_duration = *it++;
        m_trading_rate = *it++;
        m_initial_bid = *it++;
        m_initial_ask = *it++;
        m_minimum_dolar_change = *it;
    }

    /* virtual */ void ZeroIntelligence::run(std::string username) // override
    {
        cout << "--------------------\n";
        cout << "Begin of Strategy\n";
        cout << "--------------------\n";
        cout << "Strategy ID: " << m_id << '\n';
        cout << "Strategy duration: " << m_strategy_duration << '\n';
        cout << "Trading rate: " << m_trading_rate << '\n';
        cout << "Initial bid: " << m_initial_bid << '\n';
        cout << "Initial ask: " << m_initial_ask << '\n';
        cout << "Minimum dolar change: " << m_minimum_dolar_change << '\n';
        cout << "--------------------\n";
        cout << "End of Strategy\n";
        cout << "--------------------\n";
    }

} // strategies
} // shift
