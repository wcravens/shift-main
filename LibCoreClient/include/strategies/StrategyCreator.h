#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "IStrategyCreator.h"
#include "StrategyFactory.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>

namespace shift::strategies {

// helper template to simplify the process of generating strategy creators
// (one single class to rule them all)
template <typename T>
class StrategyCreator : public IStrategyCreator {
public:
    // when created, the strategy creator will automaticaly register itself with the strategy factory
    StrategyCreator(const std::string& name)
    {
        StrategyFactory::Instance().RegisterCreator(name, this);
    }

    virtual auto Create(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const -> std::unique_ptr<IStrategy> override
    {
        // create new instance of T
        // assumes T has a constructor that accepts std::initializer_list<StrategyParameter>
        return std::make_unique<T>(client, verbose, parameters);
    }
};

} // shift::strategies
