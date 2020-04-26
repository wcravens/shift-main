#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <iostream>
#include <memory>
#include <string>

namespace shift {
namespace crypto {

    /**
     * @brief General-purpose encryption tool; the counterpart of class Decryptor.
     */
    class MISCUTILS_EXPORTS Encryptor {
    public:
        Encryptor(const std::string& cryptoKey);
        Encryptor(); ///> SHA1.

        ~Encryptor();

        operator std::istream &(); ///> Enable the object transfers encrypted results as an input stream later on. E.g. enc_obj >> str;.

        friend std::istream& operator>>(std::istream& is, Encryptor& enc); ///> Input stream operator >> overloading for Encryptor.
        friend std::ostream& operator<<(std::ostream& os, Encryptor& enc); ///> Output stream operator << overloading for Encryptor.

    private:
        // implementation is hidden by Pimpl-idiom
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // crypto
} // shift
