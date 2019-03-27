#include "terminal/Common.h"

#include <mutex>

static std::mutex s_shiftTerminalMutex;

void synchPrint(const char* msg)
{
    if (cout) { // == true iff. the status flags of cout is cleared; the mutex locking incurs overhead but we don't need such locking anyway if no --verbose option set; see VerboseOptHelper's impl.
        std::lock_guard<std::mutex> guard(s_shiftTerminalMutex);
        cout << msg << flush;
    }
}

void synchPrint(const std::string& msg)
{
    synchPrint(msg.c_str());
}