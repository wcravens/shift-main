#pragma once

#include "IMarketCreator.h"
#include "MarketFactory.h"

#include <string>

namespace markets {

// helper template to simplify the process of generating market creators
// (one single class to rule them all)
template <typename T>
class MarketCreator : public IMarketCreator {
public:
    // when created, the market creator will automaticaly register itself with the market factory
    MarketCreator(const std::string& name)
    {
        MarketFactory::Instance().RegisterCreator(name, this);
    }

    virtual auto Create(const market_creator_parameters_t& parameters) const -> std::unique_ptr<Market> override
    {
        // create new instance of T
        // assumes T has a constructor that accepts std::vector<std::variant<... Types>>
        return std::make_unique<T>(parameters);
    }
};

} // markets
