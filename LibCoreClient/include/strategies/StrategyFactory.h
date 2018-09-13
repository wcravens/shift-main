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

    /// Abstract-Factory Pattern Implementation
    class StrategyFactory {
    public:
        /// Factory is implemented as a Singleton
        static StrategyFactory& Instance();

        /// Adds new strategy maker with given name
        void RegisterMaker(const std::string& name, IStrategyMaker* maker);

        /// Creates new IStrategy from initializer list
        IStrategy* Create(const std::string& name, shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const;

    private:
        StrategyFactory() = default;

        // Disable copying and assignment
        StrategyFactory(const StrategyFactory& other);
        StrategyFactory& operator=(const StrategyFactory& other);

        /// Maps names to strategy makers
        std::unordered_map<std::string, IStrategyMaker*> _makers;
    };

} // strategies
} // shift
