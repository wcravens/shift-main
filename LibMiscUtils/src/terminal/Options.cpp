#include "terminal/Options.h"

namespace shift::terminal {

/**
 * @brief Use this constructor to handle the --verbose option.
 * @param keepIfOutOfScope: If set true, then if the option is on (isVerbose == true) the effect of it will still keep/last even if this VerboseOptHelper object's lifetime ends.
 *                          By default, it is set false, which means that verbose effect is cleared immediately after the VerboseOptHelper object destroys.
 */
VerboseOptHelper::VerboseOptHelper(std::ostream& os, bool isVerbose, bool keepIfOutOfScope /*= false*/)
    : m_os { os }
    , m_isVerbose { isVerbose }
    , m_shallKeep { keepIfOutOfScope }
{
    if (m_isVerbose) {
        m_os.clear();
    } else {
        m_os.setstate(std::ios_base::failbit);
    }
}

VerboseOptHelper::~VerboseOptHelper()
{
    if (m_shallKeep) {
        return;
    }
    m_os.clear();
}

} // shift::terminal
