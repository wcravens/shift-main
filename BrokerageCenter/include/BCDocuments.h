#pragma once

#include "CandlestickData.h"
#include "ExecutionReport.h"
#include "Order.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"
#include "Parameters.h"
#include "RiskManagement.h"
#include "Transaction.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

const std::string STDSTR_NULL = "NULL";

/**
 * Some terminologies:
 * (1) "Target" or "Target Computer ID": Identifies a certain physical(FIX) connection between the BC and a remote device.
 * (2) "User", "Username", "Name", "Client", or "UserID": Identifies a certain trading user(or, "client") that remote interact with the BC.
 * (3) One (1) can consist of multiple (2)s; one (2) can interact(e.g. login) through various (1)s, but each (1) will maintain that respectively.
 */
class BCDocuments : public ITargetsInfo {
public:
    static std::atomic<bool> s_isSecurityListReady;

    static BCDocuments* getInstance();

    void addSymbol(const std::string& symbol);
    bool hasSymbol(const std::string& symbol) const;
    const std::unordered_set<std::string>& getSymbols() const; // symbols list

    void attachOrderBookToSymbol(const std::string& symbol);
    void attachCandlestickDataToSymbol(const std::string& symbol);

    void registerUserInDoc(const std::string& targetID, const std::string& userID); // for connection
    void unregisterTargetFromDoc(const std::string& targetID); // for disconnection; will unregister all connected users of this target
    std::string getTargetIDByUserID(const std::string& userID) const;

    void unregisterTargetFromOrderBooks(const std::string& targetID);
    void unregisterTargetFromCandles(const std::string& targetID);

    bool manageSubscriptionInOrderBook(bool isSubscribe, const std::string& symbol, const std::string& targetID);
    bool manageSubscriptionInCandlestickData(bool isSubscribe, const std::string& symbol, const std::string& targetID);

    int sendHistoryToUser(const std::string& userID);

    double getOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const;

    void onNewOBUpdateForOrderBook(const std::string& symbol, OrderBookEntry&& update);
    void onNewTransacForCandlestickData(const std::string& symbol, const Transaction& transac);
    void onNewOrderForUserRiskManagement(const std::string& userID, Order&& order);
    void onNewExecutionReportForUserRiskManagement(const std::string& userID, ExecutionReport&& report);

    void broadcastOrderBooks() const;

private:
    std::unordered_map<std::string, std::unique_ptr<RiskManagement>>::iterator addRiskManagementToUserNoLock(const std::string& userID);

    std::unordered_set<std::string> m_symbols;
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> m_orderBookBySymbol; // symbol, OrderBook; contains 4 types of order book for each stock
    std::unordered_map<std::string, std::unique_ptr<CandlestickData>> m_candleBySymbol; // symbol, CandlestickData; contains current price and candle data history for each stock

    mutable std::mutex m_mtxUserID2TargetID;
    std::unordered_map<std::string, std::string> m_userID2TargetID;

    mutable std::mutex m_mtxRiskManagementByUserID;
    std::unordered_map<std::string, std::unique_ptr<RiskManagement>> m_riskManagementByUserID; // userID, RiskManagement

    mutable std::mutex m_mtxOrderBookSymbolsByTargetID;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_orderBookSymbolsByTargetID; // targetID, symbols of OrderBook; for order book subscription

    mutable std::mutex m_mtxCandleSymbolsByTargetID;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_candleSymbolsByTargetID; // targetID, symbols of CandlestickData; for candle stick data subscription
};
