#pragma once

#include "CoreClient_EXPORTS.h"

#include <exception>

namespace shift {

/** 
 * @brief ConnectionTimeoutError exception for when the Brokerage Center is not accessible.
 */
struct CORECLIENT_EXPORTS ConnectionTimeoutError : public std::exception {
    const char* what() const throw()
    {
        return "Connection Timeout Error";
    }
};

/** 
 * @brief IncorrectPasswordError exception for when the user password is incorrect.
 */
struct CORECLIENT_EXPORTS IncorrectPasswordError : public std::exception {
    const char* what() const throw()
    {
        return "Incorrect Password Error";
    }
};

} // shift
