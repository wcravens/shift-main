#pragma once

#include "IMarketCreator.h"
#include "Market.h"

#include <string>
#include <unordered_map>

namespace markets {

// abstract factory pattern implementation
class MarketFactory {
public:
    // factory is implemented as a singleton
    static auto Instance() -> MarketFactory&;

    // adds new market creator with given name
    void RegisterCreator(const std::string& name, IMarketCreator* creator);

    // creates new Market from parameters
    auto Create(const std::string& name, const market_creator_parameters_t& parameters) const -> std::unique_ptr<Market>;

private:
    MarketFactory() = default; // singleton pattern
    MarketFactory(const MarketFactory& other) = delete; // forbid copying
    auto operator=(const MarketFactory& other) -> MarketFactory& = delete; // forbid assigning

    // maps names to market creators
    std::unordered_map<std::string, IMarketCreator*> m_creators;
};

} // markets
