#pragma once

#include "OrderBookEntry.h"
#include "Quote.h"
#include "Report.h"
#include "RiskManagement.h"
#include "Stock.h"
#include "StockSummary.h"
#include "Transaction.h"

#include <mutex>
#include <string>
#include <unordered_map>

const std::string STDSTR_NULL = "NULL"; // for returning-by-const-reference uses!
constexpr const char* CSTR_NULL = "NULL"; // for returning-by-value or comparison uses!

std::string toUpper(const std::string&);

class BCDocuments {
    mutable std::mutex m_mtxTransacsByID;
    mutable std::mutex m_mtxStockBySymbol;
    mutable std::mutex m_mtxCandleBySymbol;
    mutable std::mutex m_mtxSymbols;
    mutable std::mutex m_mtxID2Name; // the mutex for map from Client ID to User Name
    mutable std::mutex m_mtxName2ID; // the mutex for map from User Name to Client ID
    mutable std::mutex m_mtxOrderbookSymbolsByName;
    mutable std::mutex m_mtxCandleSymbolsByName;
    mutable std::mutex m_mtxRiskManagementByName;

    std::unordered_set<std::string> m_symbols;

    std::unordered_map<std::string, std::string> m_mapID2Name; // map from Client ID to User Name
    std::unordered_map<std::string, std::string> m_mapName2ID; // map from User Name to Client ID

    std::unordered_map<std::string, std::unique_ptr<Stock>> m_stockBySymbol; // symbol, Stock //contains 4 types of orderbook for each stock
    std::unordered_map<std::string, std::unique_ptr<StockSummary>> m_candleBySymbol; // symbol, StockSummery//contains current price and candle data history for each stock
    std::unordered_map<std::string, std::unique_ptr<RiskManagement>> m_riskManagementByName; // UserName, RiskManagement
    std::unordered_map<std::string, std::unordered_set<std::string>> m_orderbookSymbolsByName; // UserName, symbols for orderbook subscription
    std::unordered_map<std::string, std::unordered_set<std::string>> m_candleSymbolsByName; // UserName, symbols for candle stick data subscription

    auto addRiskManagementToClientNoLock(const std::string& userName) -> decltype(m_riskManagementByName)::iterator;

public:
    static BCDocuments* instance();

    double getStockOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const;
    bool manageStockOrderBookClient(bool isRegister, const std::string& symbol, const std::string& userName) const;
    bool manageCandleStickDataClient(bool isRegister, const std::string& userName, const std::string& symbol) const;
    int sendHistoryToClient(const std::string& userName);

    void addStockToSymbol(const std::string& symbol, const OrderBookEntry& ob);
    void addCandleToSymbol(const std::string& symbol, const Transaction& transac);
    void addRiskManagementToClient(const std::string& clientID, double buyingPower, int shares, double price, const std::string& symbol);
    void addQuoteToClientRiskManagement(const std::string& userID, const Quote& quote);
    void addReportToClientRiskManagement(const std::string& userID, const Report& report);

    std::unordered_map<std::string, std::string> getClientList();
    std::string getClientID(const std::string& name);
    void registerClient(const std::string& clientID, const std::string& userName); //for connection
    void unregisterClientInDocs(const std::string& userName); //for connection

    void addOrderbookSymbolToClient(const std::string& userName, const std::string& symbol);
    void addCandleSymbolToClient(const std::string& userName, const std::string& symbol);
    void removeClientFromStocks(const std::string& userName);
    void removeClientFromCandles(const std::string& userName);
    void removeCandleSymbolFromClient(const std::string& userName, const std::string& symbol);

    void addTransacToUser(const Transaction& t, const std::string& userName);
    void addSymbol(const std::string& symbol);
    bool hasSymbol(const std::string& symbol) const;
    std::unordered_set<std::string> getSymbols(); // stock list

    std::string getClientIDByName(const std::string& userName);
    std::string getClientNameByID(const std::string& clientID);

    void broadcastStocks() const;
};
