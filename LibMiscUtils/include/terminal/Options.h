#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <iostream>

namespace shift::terminal {

/**
     * @brief Utility for handling and controling command line '--verbose' option.
     */
struct MISCUTILS_EXPORTS VerboseOptHelper {
    VerboseOptHelper(std::ostream& os, bool isVerbose, bool keepIfOutOfScope = false);
    ~VerboseOptHelper();
    operator bool() { return true; } // to enable declaration in conditional statement, e.g. if(..)

private:
    std::ostream& m_os;
    bool m_isVerbose;
    bool m_shallKeep; // whether or not keep verbose/mute effection after go out of object's lifetime
};

} // shift::terminal
