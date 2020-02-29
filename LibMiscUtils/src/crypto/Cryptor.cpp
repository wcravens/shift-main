#include "Cryptor.h"

#include "SHA1.h"

shift::crypto::Cryptor::Cryptor(const std::string& key)
    : c_key(key)
{
}

shift::crypto::Cryptor::Cryptor() = default;

/**
 * @brief Do encryption or decryption, depending on isEncrypt, against the input stream, and output the operation results as input stream.
 */
std::istream& shift::crypto::Cryptor::apply(std::istream& is, bool isEncrypt)
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
std::istream& shift::crypto::Cryptor::apply(std::istream& is)
{
    SHA1 sha1;
    sha1.update(is);
    m_crypted = sha1.final();

    std::istringstream { m_crypted }.swap(m_iss);
    m_crypted.clear();

    return m_iss;
}

std::ostream& shift::crypto::Cryptor::out(std::ostream& os)
{
    if (m_iss.good()) {
        os << m_iss.str();
    }
    return os;
}

std::istream& shift::crypto::Cryptor::get()
{
    return m_iss;
}
