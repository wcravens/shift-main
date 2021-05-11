#pragma once

#include "CoreClient.h"

#include <functional>
#include <string>

#if defined(_WIN32)
#include <crossguid/Guid.h>
#else
#include <shift/miscutils/crossguid/Guid.h>
#endif

namespace shift::strategies {

// IStrategy is a public interface at the base
// of our hierarchy of strategies
class IStrategy {
public:
    // constructor attaches a client to the strategy
    IStrategy(shift::CoreClient& client, bool verbose = false)
        : m_client { client }
        , m_verbose { false }
        , m_id { shift::crossguid::newGuid().str() }
    {
    }

    // attaches a different client to the strategy
    void reattach(shift::CoreClient& client)
    {
        m_client = std::ref(client);
    }

    // runs the strategy
    virtual void run(std::string username = "") = 0;

    // more virtual functions here

    // every C++ interface should define a public virtual destructor
    // why? http://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
    virtual ~IStrategy() = default;

protected:
    std::reference_wrapper<shift::CoreClient> m_client;
    bool m_verbose;
    std::string m_id;
};

} // shift::strategies
