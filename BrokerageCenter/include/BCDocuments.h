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

/*
Some terminologies:

(1) "Target" or "Target Computer ID": Identifies a certain physical(FIX) connection between the BC and a remote device.
(2) "User", "Username", "Name" or "Client": Identifies a certain trading user(or, "client") that remote interact with the BC.
(3) One (1) can consist of multiple (2)s; one (2) can interact(e.g. login) through various (1)s, but each (1) will maintain that respectively.
*/
class BCDocuments {
    mutable std::mutex m_mtxMapsBetweenNamesAndTargetIDs; // the mutex for map from Target Computer ID to Username
    mutable std::mutex m_mtxOrderBookSymbolsByName;
    mutable std::mutex m_mtxCandleSymbolsByName;
    mutable std::mutex m_mtxRiskManagementByName;

    std::unordered_set<std::string> m_symbols;

    std::unordered_map<std::string, std::string> m_mapTarID2Name; // map from Target Computer ID to Username
    std::unordered_map<std::string, std::string> m_mapName2TarID; // map from Username to Target Computer ID

    std::unordered_map<std::string, std::unique_ptr<OrderBook>> m_orderBookBySymbol; // symbol, OrderBook; contains 4 types of order book for each stock
    std::unordered_map<std::string, std::unique_ptr<CandlestickData>> m_candleBySymbol; // symbol, CandlestickData; contains current price and candle data history for each stock
    std::unordered_map<std::string, std::unique_ptr<RiskManagement>> m_riskManagementByName; // Username, RiskManagement
    std::unordered_map<std::string, std::unordered_set<std::string>> m_orderBookSymbolsByName; // Username, symbols; for order book subscription
    std::unordered_map<std::string, std::unordered_set<std::string>> m_candleSymbolsByName; // Username, symbols; for candle stick data subscription

    auto addRiskManagementToUserNoLock(const std::string& username) -> decltype(m_riskManagementByName)::iterator;

public:
    static std::atomic<bool> s_isSecurityListReady;

    static BCDocuments* getInstance();

    double getOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const;
    bool manageUsersInOrderBook(bool isRegister, const std::string& symbol, const std::string& username) const;
    bool manageUsersInCandlestickData(bool isRegister, const std::string& username, const std::string& symbol);
    int sendHistoryToUser(const std::string& username);

    void attachOrderBookToSymbol(const std::string& symbol);
    void addOrderBookEntryToOrderBook(const std::string& symbol, const OrderBookEntry& ob);
    void attachCandlestickDataToSymbol(const std::string& symbol);
    void addTransacToCandlestickData(const std::string& symbol, const Transaction& transac);
    void addRiskManagementToUserLockedExplicit(const std::string& username, double buyingPower, int shares, double price, const std::string& symbol);
    void addOrderToUserRiskManagement(const std::string& username, const Order& order);
    void addReportToUserRiskManagement(const std::string& username, const Report& report);

    void registerUserInDoc(const std::string& username, const std::string& targetID); // for connection
    void unregisterUserInDoc(const std::string& username); // for connection
    void registerWebUserInDoc(const std::string& username, const std::string& targetID); // for connection
    std::unordered_map<std::string, std::string> getConnectedTargetIDsMap(); // Target Computer ID, Username

    void addOrderBookSymbolToUser(const std::string& username, const std::string& symbol);
    void removeUserFromOrderBooks(const std::string& username);
    void removeUserFromCandles(const std::string& username);
    void removeCandleSymbolFromUser(const std::string& username, const std::string& symbol);

    void addTransacToUser(const Transaction& t, const std::string& username);
    void addSymbol(const std::string& symbol);
    bool hasSymbol(const std::string& symbol) const;
    const std::unordered_set<std::string>& getSymbols() const; // symbols list

    std::string getTargetIDByUsername(const std::string& username);
    std::string getUsernameByTargetID(const std::string& targetID);

    void broadcastOrderBooks() const;
};
