#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace shift::crypto {

/**
 * @brief The delegated implementation class for class Encryptor and class Decryptor.
 */
struct Cryptor {
    const std::string c_key; ///> User provided key for cryption.
    std::string m_crypted; ///> Intermediate buffer for the implementation.
    std::istringstream m_iss; ///> Hold the cryption result.

    Cryptor(std::string cryptoKey); ///> For files alike.
    Cryptor() = default; ///> Using SHA1 for passwords alike.

    auto apply(std::istream& is, bool isEncrypt) -> std::istream&;
    auto apply(std::istream& is) -> std::istream&; ///> SHA1.

    auto out(std::ostream& os) -> std::ostream&;

    auto get() -> std::istream&;
};

} // shift::crypto
