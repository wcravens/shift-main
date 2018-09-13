#pragma once

#include "CoreClient.h"

#include <functional>
#include <string>

#ifdef _WIN32
#include <crossguid/Guid.h>
#else
#include <shift/miscutils/crossguid/Guid.h>
#endif

namespace shift {
namespace strategies {

    // IStrategy is a public interface at the base
    // of our hierarchy of strategies
    class IStrategy {
    public:
        // Constructor attaches a client to the strategy
        IStrategy(shift::CoreClient& client, bool verbose = false)
            : m_client(client)
            , m_verbose(false)
            , m_id(shift::crossguid::newGuid().str())
        {
        }

        // Attaches a different client to the strategy
        void reattach(shift::CoreClient& client)
        {
            m_client = std::ref(client);
        }

        // Runs the strategy
        virtual void run(std::string username = "") = 0;

        // More virtual functions here

        // Every C++ interface should define a public virtual destructor
        // Why? http://stackoverflow.com/questions/270917/why-should-i-declare-a-virtual-destructor-for-an-abstract-class-in-c
        virtual ~IStrategy() = default;

    protected:
        std::reference_wrapper<shift::CoreClient> m_client;
        bool m_verbose;
        std::string m_id;
    };

} // strategies
} // shift
