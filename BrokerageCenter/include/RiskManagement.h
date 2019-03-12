#pragma once

#include "Order.h"
#include "PortfolioItem.h"
#include "PortfolioSummary.h"
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
    // int m_userID; // internal use
    std::string m_username;

    PortfolioSummary m_porfolioSummary;
    std::unordered_map<std::string, PortfolioItem> m_portfolioItems; // Symbol, PortfolioItem
    std::unordered_map<std::string, Order> m_waitingList; // OrderID, Order
    std::queue<Order> m_orderBuffer;
    std::queue<Report> m_execRptBuffer;

    std::unordered_map<std::string, Order> m_pendingBidOrders;
    std::unordered_map<std::string, std::pair<Order, int>> m_pendingAskOrders; // pair<PendingAskOrder, ReservedShares>
    double m_pendingShortCashAmount;
    std::unordered_map<std::string, int> m_pendingShortUnitAmount;

    mutable std::mutex m_mtxPortfolioSummary;
    mutable std::mutex m_mtxPortfolioItems;
    mutable std::mutex m_mtxWaitingList;
    mutable std::mutex m_mtxOrder;
    mutable std::mutex m_mtxExecRpt;

    mutable std::mutex m_mtxOrderProcessing;

    std::unique_ptr<std::thread> m_orderThread;
    std::unique_ptr<std::thread> m_execRptThread;
    std::condition_variable m_cvOrder, m_cvExecRpt;
    std::promise<void> m_quitFlagOrder;
    std::promise<void> m_quitFlagExec;

public:
    RiskManagement(std::string username, double buyingPower);
    RiskManagement(std::string username, double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares); //> For parametric use, i.e. to explicitly configurate the initial portfolio summary.

    ~RiskManagement();

    void spawn();

    void enqueueOrder(const Order& order);
    void enqueueExecRpt(const Report& report);
    void processOrder();
    void processExecRpt();

    void insertPortfolioItem(const std::string& symbol, const PortfolioItem& portfolioItem);
    void updateWaitingList(const Report& report);

    bool verifyAndSendOrder(const Order& order);

    static void s_sendOrderToME(const Order& order);
    static void s_sendPortfolioSummaryToClient(const std::string& username, const PortfolioSummary& summary);
    static void s_sendPortfolioItemToClient(const std::string& username, const PortfolioItem& item);
    void sendPortfolioHistory();
    void sendWaitingList() const;
    double getMarketBuyPrice(const std::string& symbol);
    double getMarketSellPrice(const std::string& symbol);
};
