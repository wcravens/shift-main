#include "strategies/ZeroIntelligence.h"

#include "strategies/StrategyCreator.h"

#ifdef _WIN32
#include <terminal/Common.h>
#else
#include <shift/miscutils/terminal/Common.h>
#endif

namespace shift::strategies {

static StrategyCreator<ZeroIntelligence> creator("ZeroIntelligence");

ZeroIntelligence::ZeroIntelligence(shift::CoreClient& client, bool verbose, std::string stockTicker, double simulationDuration, double tradingRate, double initialPrice, double initialVolatility)
    : IStrategy { client, verbose }
    , m_stockTicker { std::move(stockTicker) }
    , m_simulationDuration { simulationDuration }
    , m_tradingRate { tradingRate }
    , m_initialPrice { initialPrice }
    , m_initialVolatility { initialVolatility }
{
}

ZeroIntelligence::ZeroIntelligence(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters)
    : IStrategy { client, verbose }
{
    std::initializer_list<StrategyParameter>::iterator it = parameters.begin();
    m_stockTicker = std::string(*it++);
    m_simulationDuration = *it++;
    m_tradingRate = *it++;
    m_initialPrice = *it++;
    m_initialVolatility = *it;
}

/* virtual */ void ZeroIntelligence::run(std::string username /* = "" */) // override
{
    cout << "--------------------\n";
    cout << "Begin of Strategy\n";
    cout << "--------------------\n";
    cout << "Strategy ID: " << m_id << '\n';
    cout << "Stock ticker: " << m_stockTicker << '\n';
    cout << "Strategy duration: " << m_simulationDuration << '\n';
    cout << "Trading rate: " << m_tradingRate << '\n';
    cout << "Initial price: " << m_initialPrice << '\n';
    cout << "Initial volatility: " << m_initialVolatility << '\n';
    cout << "--------------------\n";
    cout << "End of Strategy\n";
    cout << "--------------------\n";
}

} // shift::strategies
