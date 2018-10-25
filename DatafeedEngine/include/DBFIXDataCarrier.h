#pragma once

#include "FIXAcceptor.h"

#include <string>

/**
 * @brief Data structure for carrying Trading Record data between database and FIX components.
 *        Shall always use C++ standard types.
 */
struct TradingRecord {
    std::string real_time;
    // std::string execution_time;
    std::string symbol; // or RIC
    double price;
    int size;
    std::string trader_id_1;
    std::string trader_id_2;
    std::string order_id_1;
    std::string order_id_2;
    char order_type_1;
    char order_type_2;
    // std::string time_1;
    // std::string time_2;
    char decision;
    std::string destination;
    FIX::UtcTimeStamp utc_exetime;
    FIX::UtcTimeStamp utc_time1;
    FIX::UtcTimeStamp utc_time2;
};

/**
 * @brief Data structure for carrying Trade or Quote data between database and FIX components.
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
    int secs;
    double millisec;
};
