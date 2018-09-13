#pragma once

#include "OrderBookEntry.h"

#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>

/**
*  @brief Class that asynchronosly receives and/or broadcasts order books for a specific stock.
*/
class Stock {
private:
    std::string m_symbol; ///> The stock name of this Stock instance.

    mutable std::mutex m_mtxStock; ///> Mutex for Stock
    mutable std::mutex m_mtxOdrBkBuff; ///> Mutex for m_odrBkBuff.
    mutable std::mutex m_mtxStockClientList;
    mutable std::mutex m_mtxOdrBkGlobalAsk;
    mutable std::mutex m_mtxOdrBkGlobalBid;
    mutable std::mutex m_mtxOdrBkLocalAsk;
    mutable std::mutex m_mtxOdrBkLocalBid;

    bool m_flag = true;
    std::unique_ptr<std::thread> m_th;
    std::condition_variable m_cvOdrBkBuff;
    std::promise<void> m_quitFlag;

    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkGlobalAsk; // price, destination, OrderBook
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkLocalAsk;
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkGlobalBid;
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkLocalBid;

    std::queue<OrderBookEntry> m_odrBkBuff;
    std::unordered_set<std::string> m_clientList; //registered client(userName)

public:
    Stock();
    Stock(std::string name);
    ~Stock();

    void enqueueOrderBook(const OrderBookEntry& ob);
    void process();
    void spawn();
    void stop();

    void registerClientInStock(const std::string& userName);
    void unregisterClientInStock(const std::string& userName);

    void broadcastWholeOrderBookToOne(const std::string& userName);
    void broadcastWholeOrderBookToAll();
    void broadcastSingleUpdateToAll(const OrderBookEntry& record);

    void saveOrderBookGlobalBid(const OrderBookEntry& record);
    void saveOrderBookGlobalAsk(const OrderBookEntry& record);
    void saveOrderBookLocalBid(const OrderBookEntry& record);
    void saveOrderBookLocalAsk(const OrderBookEntry& record);

    double getGlobalBidOrderbookFirstPrice() const;
    double getGlobalAskOrderbookFirstPrice() const;
    double getLocalBidOrderbookFirstPrice() const;
    double getLocalAskOrderbookFirstPrice() const;
};
