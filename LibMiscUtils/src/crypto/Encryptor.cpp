#include "crypto/Encryptor.h"

#include "Cryptor.h"

namespace shift::crypto {

struct Encryptor::Impl : Cryptor {
    // using shift::crypto::Cryptor::Cryptor;
    Impl(std::string cryptoKey)
        : Cryptor { std::move(cryptoKey) }
        , m_useSHA1 { false }
    {
    }

    Impl() = default;

    bool m_useSHA1 = true;
};

Encryptor::Encryptor(std::string cryptoKey)
    : m_impl { std::make_unique<Encryptor::Impl>(std::move(cryptoKey)) }
{
}

Encryptor::Encryptor()
    : m_impl { std::make_unique<Encryptor::Impl>() }
{
}

Encryptor::~Encryptor() = default;

Encryptor::operator std::istream&()
{
    return m_impl->get();
}

// these functions are inline-declared as 'friend', hence shall be present within the same namespace

/* friend */ auto operator>>(std::istream& is, Encryptor& enc) -> std::istream&
{
    if (enc.m_impl->m_useSHA1) {
        return enc.m_impl->apply(is);
    }
    return enc.m_impl->apply(is, true);
}

/* friend */ auto operator<<(std::ostream& os, Encryptor& enc) -> std::ostream&
{
    return enc.m_impl->out(os);
}

} // shift::crypto
