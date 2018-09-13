#include "crypto/Decryptor.h"

#include "Cryptor.h"
#include "terminal/Common.h"

#include <fstream>

struct shift::crypto::Decryptor::Impl : shift::crypto::Cryptor {
    //using shift::crypto::Cryptor::Cryptor;
    Impl(const std::string& str)
        : Cryptor(str)
    {
    }
};

shift::crypto::Decryptor::Decryptor(const std::string& key)
    : m_impl(new Decryptor::Impl(key))
{
}

shift::crypto::Decryptor::~Decryptor() = default;

shift::crypto::Decryptor::operator std::istream&()
{
    return m_impl->get();
}

// these functions are inline-decleared as 'friend', hence shall be present within the same namespace
namespace shift {
namespace crypto {

    std::istream& operator>>(std::istream& is, Decryptor& dec)
    {
        return dec.m_impl->apply(is, false);
    }

    std::ostream& operator<<(std::ostream& os, Decryptor& dec)
    {
        return dec.m_impl->out(os);
    }

    std::unordered_map<std::string, std::string> readEncryptedConfigFile(const std::string& cryptoKey, const std::string& fileName, const char keyValDelim /*= '='*/)
    {
        Decryptor dec(cryptoKey);
        std::ifstream inf(fileName);
        if (inf.good())
            inf >> dec;
        else {
            cerr << '\n'
                 << COLOR_ERROR "Cannot read login file!" NO_COLOR << '\n'
                 << endl;
            return {};
        }

        std::unordered_map<std::string, std::string> info;

        auto& strm = static_cast<std::istream&>(dec); // explicitly-typed-initializer idiom
        std::string line;
        while (std::getline(strm, line)) {
            info.emplace(
                std::string(line.begin(), line.begin() + line.find(keyValDelim)), std::string(line.begin() + line.find(keyValDelim) + 1, line.end()));
        }

        return info;
    }

} // crypto
} // shift
