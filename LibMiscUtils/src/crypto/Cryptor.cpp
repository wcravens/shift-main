#include "Cryptor.h"

#include "SHA1.h"

namespace shift::crypto {

Cryptor::Cryptor(std::string cryptoKey)
    : c_key { std::move(cryptoKey) }
{
}

/**
 * @brief Do encryption or decryption, depending on isEncrypt, against the input stream, and output the operation results as input stream.
 */
auto Cryptor::apply(std::istream& is, bool isEncrypt) -> std::istream&
{
    if (!is.good()) {
        std::cerr << "The input of Encryptor object is not available." << std::endl;
        return is;
    }

    for (auto i = std::string::size_type {}; i < c_key.size(); ++i %= c_key.size()) {
        auto ch = is.get(); // this shall come first, because at the end of the istream, get() will try to consume the EOF and fails, which then set eofbit so that eof() == true

        if (is.eof()) { // encrypt done ?
            std::istringstream { m_crypted }.swap(m_iss);
            m_crypted.clear();
            return m_iss;
        }

        ch += c_key[isEncrypt ? i : 0];
        ch -= c_key[isEncrypt ? 0 : i];

        m_crypted.push_back(ch);
    }

    return is;
}

/**
 * @brief Do encryption using SHA1 against the input stream, and output the operation results as input stream.
 */
auto Cryptor::apply(std::istream& is) -> std::istream&
{
    SHA1 sha1;
    sha1.update(is);
    m_crypted = sha1.final();

    std::istringstream { m_crypted }.swap(m_iss);
    m_crypted.clear();

    return m_iss;
}

auto Cryptor::out(std::ostream& os) -> std::ostream&
{
    if (m_iss.good()) {
        os << m_iss.str();
    }
    return os;
}

auto Cryptor::get() -> std::istream&
{
    return m_iss;
}

} // shift::crypto
