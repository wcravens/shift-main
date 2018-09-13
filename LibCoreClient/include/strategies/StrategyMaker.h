#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "IStrategyMaker.h"
#include "StrategyFactory.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>

namespace shift {
namespace strategies {

    /// Helper template to simplify the process of generating strategy-maker
    template <typename T>
    class StrategyMaker : public IStrategyMaker {
    public:
        /// When created, the strategy maker will automaticaly register itself with the factory
        StrategyMaker(const std::string& name)
        {
            StrategyFactory::Instance().RegisterMaker(name, this);
        }

        virtual IStrategy* Create(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const override
        {
            // Create new instance of T
            // Assumes T has a constructor that accepts std::initializer_list<StrategyParameter>
            return new T(client, verbose, parameters);
        }
    };

} // strategies
} // shift
