#pragma once

#include "CoreClient.h"
#include "IStrategy.h"
#include "IStrategyMaker.h"
#include "StrategyParameter.h"

#include <initializer_list>
#include <string>
#include <unordered_map>

namespace shift {
namespace strategies {

    // abstract-Factory Pattern Implementation
    class StrategyFactory {
    public:
        // factory is implemented as a Singleton
        static StrategyFactory& Instance();

        // adds new strategy maker with given name
        void RegisterMaker(const std::string& name, IStrategyMaker* maker);

        // creates new IStrategy from initializer list
        IStrategy* Create(const std::string& name, shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const;

    private:
        StrategyFactory() = default;

        // disable copying and assignment
        StrategyFactory(const StrategyFactory& other);
        StrategyFactory& operator=(const StrategyFactory& other);

        // maps names to strategy makers
        std::unordered_map<std::string, IStrategyMaker*> _makers;
    };

} // strategies
} // shift
