#pragma once

#include "PortfolioItem.h"
#include "PortfolioSummary.h"
#include "Quote.h"
#include "Report.h"

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

class RiskManagement {
private:
    int m_userID; // internal use
    std::string m_userName;

    PortfolioSummary m_porfolioSummary;
    std::unordered_map<std::string, PortfolioItem> m_portfolioItems; // Symbol, PortfolioItem
    std::unordered_map<std::string, Quote> m_quoteHistory; // OrderID, Quote
    std::queue<Quote> m_quoteBuffer;
    std::queue<Report> m_execRptBuffer;

    std::unordered_map<std::string, Quote> m_pendingBidOrders;
    std::unordered_map<std::string, std::pair<Quote, int>> m_pendingAskOrders; // pair<PendingAskOrder, ReservedShares>
    double m_pendingShortCashAmount;
    std::unordered_map<std::string, int> m_pendingShortUnitAmount;

    mutable std::mutex m_mtxPortfolioSummary;
    mutable std::mutex m_mtxPortfolioItems;
    mutable std::mutex m_mtxQuoteHistory;
    mutable std::mutex m_mtxQuote;
    mutable std::mutex m_mtxExecRpt;

    mutable std::mutex m_mtxQuoteProcessing;

    std::unique_ptr<std::thread> m_quoteThread;
    std::unique_ptr<std::thread> m_execRptThread;
    std::condition_variable m_cvQuote, m_cvExecRpt;
    std::promise<void> m_quitFlagQuote;
    std::promise<void> m_quitFlagExec;

public:
    RiskManagement(std::string userName, double buyingPower);
    RiskManagement(std::string userName, double buyingPower, int totalShares); //> For default use, i.e. portfolio summary's Holding Balance/Borrowed Balance/Total P&L == 0.
    RiskManagement(std::string userName, double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares); //> For parametric use, i.e. to explicitly configurate the initial portfolio summary.

    ~RiskManagement();

    void spawn();

    void enqueueQuote(const Quote& quote);
    void enqueueExecRpt(const Report& report);
    void processQuote();
    void processExecRpt();

    void insertPortfolioItem(const std::string& symbol, const PortfolioItem& portfolioItem);
    void updateQuoteHistory(const Report& report);

    bool verifyAndSendQuote(const Quote& quote);

    static void s_sendQuoteToME(const Quote& quote);
    static void s_sendPortfolioSummaryToClient(const std::string& userName, const PortfolioSummary& summary);
    static void s_sendPortfolioItemToClient(const std::string& userName, const PortfolioItem& item);
    void sendPortfolioHistory();
    void sendQuoteHistory() const;
    double getMarketBuyPrice(const std::string& symbol);
    double getMarketSellPrice(const std::string& symbol);
};
