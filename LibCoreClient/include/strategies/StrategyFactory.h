#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "IStrategyMaker.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>
#include <unordered_map>

namespace shift::strategies {

// abstract-Factory Pattern Implementation
class StrategyFactory {
public:
    // factory is implemented as a Singleton
    static auto Instance() -> StrategyFactory&;

    // adds new strategy maker with given name
    void RegisterMaker(const std::string& name, IStrategyMaker* maker);

    // creates new IStrategy from initializer list
    auto Create(const std::string& name, shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const -> IStrategy*;

private:
    StrategyFactory() = default; // singleton pattern
    StrategyFactory(const StrategyFactory& other) = delete; // forbid copying
    auto operator=(const StrategyFactory& other) -> StrategyFactory& = delete; // forbid assigning

    // maps names to strategy makers
    std::unordered_map<std::string, IStrategyMaker*> _makers;
};

} // shift::strategies
