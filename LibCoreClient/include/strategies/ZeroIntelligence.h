#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>

namespace shift {
namespace strategies {

    class ZeroIntelligence : public IStrategy {
    public:
        ZeroIntelligence(shift::CoreClient& client, bool verbose, int strategy_duration, int trading_rate, double initial_bid, double initial_ask, double minimum_dolar_change);
        ZeroIntelligence(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& v);

        virtual void run(std::string username = "") override;

    private:
        int m_strategy_duration;
        int m_trading_rate;
        double m_initial_bid;
        double m_initial_ask;
        double m_minimum_dolar_change;
    };

} // strategies
} // shift
