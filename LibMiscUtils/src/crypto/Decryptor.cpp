#include "crypto/Decryptor.h"

#include "Cryptor.h"
#include "terminal/Common.h"

#include <fstream>

namespace shift::crypto {

struct Decryptor::Impl : Cryptor {
    // using shift::crypto::Cryptor::Cryptor;
    Impl(std::string cryptoKey)
        : Cryptor { std::move(cryptoKey) }
    {
    }
};

Decryptor::Decryptor(std::string cryptoKey)
    : m_impl { std::make_unique<Decryptor::Impl>(std::move(cryptoKey)) }
{
}

Decryptor::~Decryptor() = default;

Decryptor::operator std::istream &()
{
    return m_impl->get();
}

// these functions are inline-declared as 'friend', hence shall be present within the same namespace

auto operator>>(std::istream& is, Decryptor& dec) -> std::istream&
{
    return dec.m_impl->apply(is, false);
}

auto operator<<(std::ostream& os, Decryptor& dec) -> std::ostream&
{
    return dec.m_impl->out(os);
}

auto readEncryptedConfigFile(const std::string& cryptoKey, const std::string& fileName, const char keyValDelim /*= '='*/) -> std::unordered_map<std::string, std::string>
{
    Decryptor dec(cryptoKey);
    std::ifstream inf(fileName);
    if (inf.good()) {
        inf >> dec;
    } else {
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

} // shift::crypto
