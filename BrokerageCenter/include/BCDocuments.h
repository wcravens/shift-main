#pragma once

#include "CandlestickData.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"
#include "Report.h"
#include "RiskManagement.h"
#include "Transaction.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

const std::string STDSTR_NULL = "NULL";

std::string toUpper(const std::string&);
std::string toLower(const std::string&);

/*
Some terminologies:

(1) "Target" or "Target Computer ID": Identifies a certain physical(FIX) connection between the BC and a remote device.
(2) "User", "Username", "Name" or "Client": Identifies a certain trading user(or, "client") that remote interact with the BC.
(3) One (1) can consist of multiple (2)s; one (2) can interact(e.g. login) through various (1)s, but each (1) will maintain that respectively.
*/
class BCDocuments : public ITargetsInfo {
    std::unordered_set<std::string> m_symbols;
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> m_orderBookBySymbol; // symbol, OrderBook; contains 4 types of order book for each stock
    std::unordered_map<std::string, std::unique_ptr<CandlestickData>> m_candleBySymbol; // symbol, CandlestickData; contains current price and candle data history for each stock

    mutable std::mutex m_mtxUserTargetInfo;
    std::unordered_map<std::string, std::string> m_mapName2TarID; // map from Username to Target Computer ID

    mutable std::mutex m_mtxRiskManagementByName;
    std::unordered_map<std::string, std::unique_ptr<RiskManagement>> m_riskManagementByName; // Username, RiskManagement

    mutable std::mutex m_mtxOrderBookSymbolsByTargetID;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_orderBookSymbolsByTargetID; // Username, symbols; for order book subscription

    mutable std::mutex m_mtxCandleSymbolsByTargetID;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_candleSymbolsByTargetID; // Username, symbols; for candle stick data subscription

    auto addRiskManagementToUserNoLock(const std::string& username) -> decltype(m_riskManagementByName)::iterator;

public:
    static BCDocuments* getInstance();

    static std::atomic<bool> s_isSecurityListReady;

    void addSymbol(const std::string& symbol);
    bool hasSymbol(const std::string& symbol) const;
    const std::unordered_set<std::string>& getSymbols() const; // symbols list

    void attachOrderBookToSymbol(const std::string& symbol);
    void attachCandlestickDataToSymbol(const std::string& symbol);

    void registerUserInDoc(const std::string& targetID, const std::string& username); // for connection
    void unregisterTargetFromDoc(const std::string& targetID); // for disconnection; will unregister all connected users of this target
    std::string getTargetIDByUsername(const std::string& username) const;

    void unregisterTargetFromOrderBooks(const std::string& targetID);
    void unregisterTargetFromCandles(const std::string& targetID);

    bool manageSubscriptionInOrderBook(bool isSubscribe, const std::string& symbol, const std::string& targetID);
    bool manageSubscriptionInCandlestickData(bool isSubscribe, const std::string& symbol, const std::string& targetID);

    int sendHistoryToUser(const std::string& username);

    double getOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const;

    void onNewOBEntryForOrderBook(const std::string& symbol, const OrderBookEntry& entry);
    void onNewTransacForCandlestickData(const std::string& symbol, const Transaction& transac);
    void onNewOrderForUserRiskManagement(const std::string& username, Order&& order);
    void onNewReportForUserRiskManagement(const std::string& username, const Report& report);

    void broadcastOrderBooks() const;
};
