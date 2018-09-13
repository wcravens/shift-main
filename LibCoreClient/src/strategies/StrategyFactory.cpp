#include "strategies/StrategyFactory.h"

#include <stdexcept>

namespace shift {
namespace strategies {

    // public:

    StrategyFactory& StrategyFactory::Instance()
    {
        // So called Meyers Singleton implementation,
        // In C++ 11 it is in fact thread-safe
        // In older versions you should ensure thread-safety here
        static StrategyFactory factory;
        return factory;
    }

    void StrategyFactory::RegisterMaker(const std::string& name, IStrategyMaker* maker)
    {
        // Validate uniqueness and add to the map
        if (_makers.find(name) != _makers.end()) {
            throw new std::runtime_error("Multiple makers for given name!");
        }
        _makers[name] = maker;
    }

    IStrategy* StrategyFactory::Create(const std::string& name, shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const
    {
        // Look up the maker by strategy name
        auto i = _makers.find(name);
        if (i == _makers.end()) {
            throw new std::runtime_error("Unrecognized object type!");
        }
        IStrategyMaker* maker = i->second;
        // Invoke create polymorphicaly
        return maker->Create(client, verbose, parameters);
    }

} // strategies
} // shift
