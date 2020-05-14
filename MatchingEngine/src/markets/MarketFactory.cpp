#include "markets/MarketFactory.h"

#include <stdexcept>

namespace markets {

/* static */ auto MarketFactory::Instance() -> MarketFactory&
{
    // so called Meyers Singleton implementation:
    // in C++ 11 it is in fact thread-safe,
    // in older versions you should ensure thread-safety here
    static MarketFactory factory;
    return factory;
}

void MarketFactory::RegisterCreator(const std::string& name, IMarketCreator* creator)
{
    // validate uniqueness and add to the map
    if (m_creators.find(name) != m_creators.end()) {
        throw new std::runtime_error("Multiple creators for given name!");
    }
    m_creators[name] = creator;
}

auto MarketFactory::Create(const std::string& name, const market_creator_parameters_t& parameters) const -> std::unique_ptr<Market>
{
    // look up the creator by market name
    auto i = m_creators.find(name);
    if (i == m_creators.end()) {
        throw new std::runtime_error("Unrecognized object type!");
    }
    IMarketCreator* creator = i->second;
    // invoke create polymorphicaly
    return creator->Create(parameters);
}

} // markets
