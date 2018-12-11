#include "crypto/Encryptor.h"

#include "Cryptor.h"

struct shift::crypto::Encryptor::Impl : shift::crypto::Cryptor {
    //using shift::crypto::Cryptor::Cryptor;
    Impl(const std::string& cryptoKey)
        : Cryptor(cryptoKey)
        , m_useSHA1(false)
    {
    }

    Impl() = default;

    bool m_useSHA1 = true;
};

shift::crypto::Encryptor::Encryptor(const std::string& cryptoKey)
    : m_impl(new Encryptor::Impl(cryptoKey))
{
}

shift::crypto::Encryptor::Encryptor()
    : m_impl(new Encryptor::Impl)
{
}

shift::crypto::Encryptor::~Encryptor() = default;

shift::crypto::Encryptor::operator std::istream&()
{
    return m_impl->get();
}

// these functions are inline-decleared as 'friend', hence shall be present within the same namespace
namespace shift {
namespace crypto {

    std::istream& operator>>(std::istream& is, Encryptor& enc)
    {
        if (enc.m_impl->m_useSHA1)
            return enc.m_impl->apply(is);
        return enc.m_impl->apply(is, true);
    }

    std::ostream& operator<<(std::ostream& os, Encryptor& enc)
    {
        return enc.m_impl->out(os);
    }

} // crypto
} // shift
