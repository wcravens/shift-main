#include "crypto/Encryptor.h"

#include "Cryptor.h"

struct shift::crypto::Encryptor::Impl : shift::crypto::Cryptor {
    //using shift::crypto::Cryptor::Cryptor;
    Impl(const std::string& str)
        : Cryptor(str)
    {
    }
};

shift::crypto::Encryptor::Encryptor(const std::string& key)
    : m_impl(new Encryptor::Impl(key))
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
        return enc.m_impl->apply(is, true);
    }

    std::ostream& operator<<(std::ostream& os, Encryptor& enc)
    {
        return enc.m_impl->out(os);
    }

} // crypto
} // shift
