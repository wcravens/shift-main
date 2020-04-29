#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>

namespace shift::strategies {

class ZeroIntelligence : public IStrategy {
public:
    ZeroIntelligence(shift::CoreClient& client, bool verbose, std::string stockTicker, double simulationDuration, double tradingRate, double initialPrice, double initialVolatility);
    ZeroIntelligence(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters);

    virtual void run(std::string username = "") override;

private:
    std::string m_stockTicker;
    double m_simulationDuration;
    double m_tradingRate;
    double m_initialPrice;
    double m_initialVolatility;
};

} // shift::strategies
