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

/**
 * @brief Data structure for carrying Trade or Quote data 
 *        between database and FIX components.
 *        Shall always use C++ standard types.
 */
struct RawData {
    std::string symbol; // or RIC
    std::string reutersDate;
    std::string reutersTime;
    std::string toq; // Trades-Or-Quotes
    std::string exchangeID; // or QuoteID
    double price;
    int volume;
    std::string buyerID; // or QuoteID
    double bidPrice;
    int bidSize;
    std::string sellerID; // or QuoteReqID
    double askPrice;
    int askSize;
    std::string exchangeTime;
    std::string quoteTime;
    std::string recordID;
    int secs; // second of date since 1970.01.01
    double microsecs; // millicsecond of one day
};
