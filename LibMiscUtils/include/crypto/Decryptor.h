#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace shift {
namespace crypto {

    /*
    * @brief General-purpose decryption tool; the counterpart of class Encryptor.
    */
    class MISCUTILS_EXPORTS Decryptor {
    public:
        Decryptor(const std::string& cryptoKey);

        ~Decryptor();

        operator std::istream&(); ///> Enable the object transfers decrypted results as an input stream later on. E.g. dec_obj >> str;.

        friend std::istream& operator>>(std::istream& is, Decryptor& dec); ///> Input stream operator >> overloading for Decryptor.
        friend std::ostream& operator<<(std::ostream& os, Decryptor& dec); ///> Output stream operator << overloading for Decryptor.

    private:
        // Implementation is hidden by Pimpl-idiom.
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    MISCUTILS_EXPORTS std::unordered_map<std::string, std::string> readEncryptedConfigFile(const std::string& cryptoKey, const std::string& fileName, const char keyValDelim = '=');

} // crypto
} // shift
