#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "StrategyParameter.h"

#include <initializer_list>

namespace shift::strategies {

// IStrategyCreator is a public parent of all strategy creators (represented by the single template class StrategyCreator)
// it represents a function to be invoked when creating a new strategy type
class IStrategyCreator {
public:
    // accepts a initializer list of StrategyParameter to pass into strategy constructors
    // returns new strategy object
    virtual auto Create(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const -> std::unique_ptr<IStrategy> = 0;

    // every C++ interface should define a public virtual destructor
    // why? http://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
    virtual ~IStrategyCreator() = default;
};

} // shift::strategies
