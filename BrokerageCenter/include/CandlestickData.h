#pragma once

#include "CandlestickDataPoint.h"
#include "Interfaces.h"
#include "Transaction.h"

#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class CandlestickData : public ITargetsInfo {
public:
    CandlestickData(); //> This will postpone initializing of valid startup status (e.g. symbol is not empty and open time is not 0) to first time processing a transaction.
    CandlestickData(const std::string& symbol, double currPrice, double currOpenPrice, double currClosePrice, double currHighPrice, double currLowPrice, std::time_t currOpenTime);
    ~CandlestickData() override;

    void sendPoint(const CandlestickDataPoint& cdPoint);
    void sendHistory(std::string targetID);

    const std::string& getSymbol() const;

    static std::time_t s_nowUnixTimestamp() noexcept;
    static std::time_t s_toUnixTimestamp(const std::string& time) noexcept;

    void registerUserInCandlestickData(const std::string& targetID);
    void unregisterUserInCandlestickData(const std::string& targetID);
    void enqueueTransaction(const Transaction& t);
    void process();
    void spawn();

private:
    std::string m_symbol;
    double m_lastOpenPrice;
    double m_lastClosePrice;
    double m_lastHighPrice;
    double m_lastLowPrice;
    std::time_t m_lastOpenTime;

    std::queue<Transaction> m_transacBuff;
    std::map<std::time_t, CandlestickDataPoint> m_history;

    mutable std::mutex m_mtxTransacBuff;
    mutable std::mutex m_mtxHistory;

    std::unique_ptr<std::thread> m_th;
    std::condition_variable m_cvCD;
    std::promise<void> m_quitFlag;
};
