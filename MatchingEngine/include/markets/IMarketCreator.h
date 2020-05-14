#pragma once

#include "Market.h"

#include <string>
#include <variant>
#include <vector>

namespace markets {

using market_creator_parameters_t = std::vector<std::variant<std::string, double>>;

// IMarketCreator is the public parent of all market creators (represented by the single template class MarketCreator)
// it represents a function to be invoked when creating a new market type
class IMarketCreator {
public:
    // accepts a vector of variant parameters to pass into market constructors
    // returns new market object
    virtual auto Create(const market_creator_parameters_t& parameters) const -> std::unique_ptr<Market> = 0;

    // every C++ interface should define a public virtual destructor
    // why? http://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
    virtual ~IMarketCreator() = default;
};

} // markets
