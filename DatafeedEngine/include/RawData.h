#pragma once

#include <string>

#include <quickfix/FieldTypes.h>

/**
 * @brief Data structure for carrying Trade or Quote data between database and FIX components.
 *        Shall always use C++ standard types.
 */
struct RawData {
    std::string symbol; // or RIC
    std::string reutersDate;
    std::string reutersTime;
    std::string toq; // trade or quote
    std::string exchangeID;
    double price;
    int volume;
    std::string buyerID;
    double bidPrice;
    int bidSize;
    std::string sellerID;
    double askPrice;
    int askSize;
    std::string exchangeTime;
    std::string quoteTime;
    std::string recordID;
    int secs; // seconds since 1970/01/01
    double microsecs; // microseconds of the day
};
