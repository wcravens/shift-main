#pragma once

#include <iostream>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::flush;

#define COLOR \
    "\033[1;36m" // bold;cyan
#define COLOR_WARNING \
    "\033[1;33m" // bold;yellow
#define COLOR_PROMPT \
    "\033[1;32m" // bold;green
#define COLOR_ERROR \
    "\033[1;31m" // bold;red
#define NO_COLOR \
    "\033[0m"

// guaranteed interleaving-free printing
void synchPrint(const char* msg);
void synchPrint(const std::string& msg);
