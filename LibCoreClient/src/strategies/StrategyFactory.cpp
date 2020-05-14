#include "strategies/StrategyFactory.h"

#include <stdexcept>

namespace shift::strategies {

/* static */ auto StrategyFactory::Instance() -> StrategyFactory&
{
    // so called Meyers Singleton implementation:
    // in C++ 11 it is in fact thread-safe,
    // in older versions you should ensure thread-safety here
    static StrategyFactory factory;
    return factory;
}

void StrategyFactory::RegisterCreator(const std::string& name, IStrategyCreator* creator)
{
    // validate uniqueness and add to the map
    if (m_creators.find(name) != m_creators.end()) {
        throw new std::runtime_error("Multiple creators for given name!");
    }
    m_creators[name] = creator;
}

auto StrategyFactory::Create(const std::string& name, shift::CoreClient& client, bool verbose, const std::initializer_list<StrategyParameter>& parameters) const -> std::unique_ptr<IStrategy>
{
    // look up the creator by strategy name
    auto i = m_creators.find(name);
    if (i == m_creators.end()) {
        throw new std::runtime_error("Unrecognized object type!");
    }
    IStrategyCreator* creator = i->second;
    // invoke create polymorphicaly
    return creator->Create(client, verbose, parameters);
}

} // shift::strategies
