#pragma once

#include "CoreClient_EXPORTS.h"

#include <exception>

namespace shift {

/** 
 * @brief ConnectionTimeout exception for when the Brokerage Center is not accessible.
 */
struct CORECLIENT_EXPORTS ConnectionTimeout : public std::exception {
    const char* what() const throw()
    {
        return "Connection Timeout";
    }
};

/** 
 * @brief IncorrectPassword exception for when the user password is incorrect.
 */
struct CORECLIENT_EXPORTS IncorrectPassword : public std::exception {
    const char* what() const throw()
    {
        return "Incorrect Password";
    }
};

} // shift
