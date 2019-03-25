#include "terminal/Common.h"

#include <mutex>

static std::mutex s_shiftTerminalMutex;

void synchPrint(const char* msg)
{
    std::lock_guard<std::mutex> guard(s_shiftTerminalMutex);
    cout << msg << flush;
}

void synchPrint(const std::string& msg)
{
    synchPrint(msg.c_str());
}