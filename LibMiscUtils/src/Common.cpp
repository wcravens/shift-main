#include "Common.h"

#include <algorithm>
#include <ctype.h>
#include <iterator>

namespace shift {

std::string toUpper(const std::string& str)
{
    std::string upStr;
    std::transform(str.begin(), str.end(), std::back_inserter(upStr), [](char ch) { return std::toupper(ch); });
    return upStr;
}

std::string toLower(const std::string& str)
{
    std::string loStr;
    std::transform(str.begin(), str.end(), std::back_inserter(loStr), [](char ch) { return std::tolower(ch); });
    return loStr;
}

} // namespace shift