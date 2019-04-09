#pragma once

#include "quickfix/FieldTypes.h"

#include <string>

/**
 * @brief Data structure for carrying Trading Record data
 *        between database and FIX components.
 *        Shall always use C++ standard types.
 */
struct TradingRecord {
    FIX::UtcTimeStamp realTime;
    FIX::UtcTimeStamp execTime;
    std::string symbol; // or RIC
    double price;
    int size;
    std::string traderID1;
    std::string traderID2;
    std::string orderID1;
    std::string orderID2;
    char orderType1;
    char orderType2;
    char decision;
    std::string destination;
    FIX::UtcTimeStamp time1;
    FIX::UtcTimeStamp time2;
};
