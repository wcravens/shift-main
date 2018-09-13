#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "StrategyParameter.h"

#include <initializer_list>

namespace shift {
namespace strategies {

    // IStrategyMaker is a public parent of all strategy makers
    // It represents a function to be invoked when creating a new strategy
    class IStrategyMaker {
    public:
        /// Accepts a vector of StrategyParameter to pass into strategy constructors
        /// Returns new strategy object
        virtual IStrategy* Create(shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const = 0;

        // Every C++ interface should define a public virtual destructor
        // Why? http://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
        virtual ~IStrategyMaker() = default;
    };

} // strategies
} // shift
