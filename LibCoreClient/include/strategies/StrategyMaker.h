#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "IStrategyMaker.h"
#include "StrategyFactory.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>

namespace shift::strategies {

// helper template to simplify the process of generating strategy-maker
template <typename T>
class StrategyMaker : public IStrategyMaker {
public:
    // when created, the strategy maker will automaticaly register itself with the factory
    StrategyMaker(const std::string& name)
    {
        StrategyFactory::Instance().RegisterMaker(name, this);
    }

    virtual auto Create(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const -> IStrategy* override
    {
        // create new instance of T
        // assumes T has a constructor that accepts std::initializer_list<StrategyParameter>
        return new T(client, verbose, parameters);
    }
};

} // shift::strategies
