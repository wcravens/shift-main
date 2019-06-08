#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "StrategyParameter.h"

#include <initializer_list>

namespace shift {
namespace strategies {

    // IStrategyMaker is a public parent of all strategy makers
    // it represents a function to be invoked when creating a new strategy
    class IStrategyMaker {
    public:
        // accepts a vector of StrategyParameter to pass into strategy constructors
        // returns new strategy object
        virtual IStrategy* Create(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const = 0;

        // every C++ interface should define a public virtual destructor
        // why? http://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
        virtual ~IStrategyMaker() = default;
    };

} // strategies
} // shift
