#pragma once

#include "TempStockSummary.h"
#include "Transaction.h"

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>

class StockSummary {
private:
    std::string m_symbol;
    double m_currPrice;
    double m_currOpenPrice;
    double m_currClosePrice;
    double m_currHighPrice;
    double m_currLowPrice;
    std::time_t m_currOpenTime;
    bool m_lastTransacSent;

    std::queue<Transaction> m_transacBuff;
    std::atomic<size_t> m_tranBufSizeAtom; // For performance purpose: lock-free fast querying of transaction buffer size

    std::unordered_set<std::string> m_clientList; // userName
    std::map<std::time_t, TempStockSummary> m_history;

    mutable std::mutex m_mtxTransacBuff;
    mutable std::mutex m_mtxSSClientList;
    mutable std::mutex m_mtxHistory;

    std::unique_ptr<std::thread> m_th;
    std::condition_variable m_cvSS;
    std::promise<void> m_quitFlag;

public:
    StockSummary();
    StockSummary(std::string symbol, double currPrice, double currOpenPrice, double currClosePrice, double currHighPrice, double currLowPrice, std::time_t currOpenTime);
    ~StockSummary();

    void sendCurrentStockSummary(const TempStockSummary& tempSS);
    void sendHistory(const std::string userName);

    const std::string& getSymbol() const;

    static std::time_t s_nowUnixTimestamp() noexcept;
    static std::time_t s_toUnixTimestamp(const std::string& time) noexcept;

    void registerClientInSS(const std::string& userName);
    void unregisterClientInSS(const std::string& userName);
    void enqueueTransaction(const Transaction& t);
    void process();
    void spawn();
};
