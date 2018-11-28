#pragma once

#include "OrderBookEntry.h"
#include "Quote.h"
#include "Report.h"
#include "RiskManagement.h"
#include "Stock.h"
#include "CandlestickData.h"
#include "Transaction.h"

#include <mutex>
#include <string>
#include <unordered_map>

const std::string STDSTR_NULL = "NULL"; // for returning-by-const-reference uses!
constexpr const char* CSTR_NULL = "NULL"; // for returning-by-value or comparison uses!

std::string toUpper(const std::string&);

/*
Some terminologies:

(1) "Target" or "Target Computer ID": Identifies a certain physical(FIX) connection between the BC and a remote device.
(2) "User", "User Name", "Name" or "Client": Identifies a certain trading user(or, "client") that remote interact with the BC.
(3) One (1) can consist of multiple (2)s; one (2) can interact(e.g. login) through various (1)s, but each (1) will maintain that respectively.
*/
class BCDocuments {
    mutable std::mutex m_mtxTransacsByID;
    mutable std::mutex m_mtxStockBySymbol;
    mutable std::mutex m_mtxCandleBySymbol;
    mutable std::mutex m_mtxSymbols;
    mutable std::mutex m_mtxTarID2Name; // the mutex for map from Target Computer ID to User Name
    mutable std::mutex m_mtxName2TarID; // the mutex for map from User Name to Target Computer ID
    mutable std::mutex m_mtxOrderbookSymbolsByName;
    mutable std::mutex m_mtxCandleSymbolsByName;
    mutable std::mutex m_mtxRiskManagementByName;

    std::unordered_set<std::string> m_symbols;

    std::unordered_map<std::string, std::string> m_mapTarID2Name; // map from Target Computer ID to User Name
    std::unordered_map<std::string, std::string> m_mapName2TarID; // map from User Name to Target Computer ID

    std::unordered_map<std::string, std::unique_ptr<Stock>> m_stockBySymbol; // symbol, Stock; contains 4 types of orderbook for each stock
    std::unordered_map<std::string, std::unique_ptr<CandlestickData>> m_candleBySymbol; // symbol, StockSummery; contains current price and candle data history for each stock
    std::unordered_map<std::string, std::unique_ptr<RiskManagement>> m_riskManagementByName; // UserName, RiskManagement
    std::unordered_map<std::string, std::unordered_set<std::string>> m_orderbookSymbolsByName; // UserName, symbols; for orderbook subscription
    std::unordered_map<std::string, std::unordered_set<std::string>> m_candleSymbolsByName; // UserName, symbols; for candle stick data subscription

    auto addRiskManagementToUserNoLock(const std::string& userName) -> decltype(m_riskManagementByName)::iterator;

public:
    static BCDocuments* instance();

    double getStockOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const;
    bool manageStockOrderBookUser(bool isRegister, const std::string& symbol, const std::string& userName) const;
    bool manageCandleStickDataUser(bool isRegister, const std::string& userName, const std::string& symbol) const;
    int sendHistoryToUser(const std::string& userName);

    void addStockToSymbol(const std::string& symbol, const OrderBookEntry& ob);
    void addCandleToSymbol(const std::string& symbol, const Transaction& transac);
    void addRiskManagementToUser(const std::string& userName, double buyingPower, int shares, double price, const std::string& symbol);
    void addQuoteToUserRiskManagement(const std::string& userName, const Quote& quote);
    void addReportToUserRiskManagement(const std::string& userName, const Report& report);

    std::unordered_map<std::string, std::string> getUserList(); // UserName, Target Computer ID
    std::string getTargetID(const std::string& userName);
    void registerUserInDoc(const std::string& targetID, const std::string& userName); // for connection
    void unregisterUserInDoc(const std::string& userName); // for connection

    void addOrderbookSymbolToUser(const std::string& userName, const std::string& symbol);
    void addCandleSymbolToUser(const std::string& userName, const std::string& symbol);
    void removeUserFromStocks(const std::string& userName);
    void removeUserFromCandles(const std::string& userName);
    void removeCandleSymbolFromUser(const std::string& userName, const std::string& symbol);

    void addTransacToUser(const Transaction& t, const std::string& userName);
    void addSymbol(const std::string& symbol);
    bool hasSymbol(const std::string& symbol) const;
    std::unordered_set<std::string> getSymbols(); // stock list

    std::string getTargetIDByUserName(const std::string& userName);
    std::string getUserNameByTargetID(const std::string& targetID);

    void broadcastStocks() const;
};
