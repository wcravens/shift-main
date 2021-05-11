#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace shift::crypto {

/**
 * @brief General-purpose decryption tool; the counterpart of class Encryptor.
 */
class MISCUTILS_EXPORTS Decryptor {
public:
    Decryptor(std::string cryptoKey);

    ~Decryptor();

    operator std::istream&(); ///> Enable the object transfers decrypted results as an input stream later on. E.g. dec_obj >> str;.

    friend auto operator>>(std::istream& is, Decryptor& dec) -> std::istream&; ///> Input stream operator >> overloading for Decryptor.
    friend auto operator<<(std::ostream& os, Decryptor& dec) -> std::ostream&; ///> Output stream operator << overloading for Decryptor.

private:
    // implementation is hidden by Pimpl-idiom
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

MISCUTILS_EXPORTS auto readEncryptedConfigFile(const std::string& cryptoKey, const std::string& fileName, const char keyValDelim = '=') -> std::unordered_map<std::string, std::string>;

} // shift::crypto
