#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "IStrategyCreator.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>
#include <unordered_map>

namespace shift::strategies {

// abstract factory pattern implementation
class StrategyFactory {
public:
    // factory is implemented as a Singleton
    static auto Instance() -> StrategyFactory&;

    // adds new strategy creator with given name
    void RegisterCreator(const std::string& name, IStrategyCreator* creator);

    // creates new IStrategy from parameters
    auto Create(const std::string& name, shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const -> std::unique_ptr<IStrategy>;

private:
    StrategyFactory() = default; // singleton pattern
    StrategyFactory(const StrategyFactory& other) = delete; // forbid copying
    auto operator=(const StrategyFactory& other) -> StrategyFactory& = delete; // forbid assigning

    // maps names to strategy creators
    std::unordered_map<std::string, IStrategyCreator*> m_creators;
};

} // shift::strategies
