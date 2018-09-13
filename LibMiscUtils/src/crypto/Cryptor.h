#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace shift {
namespace crypto {

    /*
    * @brief The delegated implementation class for class Encryptor and class Decryptor.
    */
    struct Cryptor {
        const std::string c_key; ///> User provided key for cryption.
        std::string m_crypted; ///> Intermediate buffer for the implementation.
        std::istringstream m_iss; ///> Hold the cryption result.

        Cryptor(const std::string& key);

        std::istream& apply(std::istream& is, bool isEncrypt);

        std::ostream& out(std::ostream& os);

        std::istream& get();
    };

} // crypto
} // shift
