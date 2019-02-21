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

/**
*  @brief Class that asynchronosly receives and/or broadcasts order books for a specific stock.
*/
class Stock {
private:
    std::string m_symbol; ///> The stock name of this Stock instance.

    mutable std::mutex m_mtxOdrBkBuff; ///> Mutex for m_odrBkBuff.
    mutable std::mutex m_mtxStockUserList;
    mutable std::mutex m_mtxOdrBkGlobalAsk;
    mutable std::mutex m_mtxOdrBkGlobalBid;
    mutable std::mutex m_mtxOdrBkLocalAsk;
    mutable std::mutex m_mtxOdrBkLocalBid;

    std::unique_ptr<std::thread> m_th;
    std::condition_variable m_cvOdrBkBuff;
    std::promise<void> m_quitFlag;

    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkGlobalAsk; // price, destination, order book entry
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkLocalAsk;
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkGlobalBid;
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkLocalBid;

    std::queue<OrderBookEntry> m_odrBkBuff;
    std::vector<std::string> m_stockUserList; // names of registered users for this Stock

public:
    Stock();
    Stock(std::string name);
    ~Stock();

    void enqueueOrderBook(const OrderBookEntry& ob);
    void process();
    void spawn();
    // void stop();

    void registerUserInStock(const std::string& userName);
    void unregisterUserInStock(const std::string& userName);

    void broadcastWholeOrderBookToOne(const std::string& userName);
    void broadcastWholeOrderBookToAll();
    void broadcastSingleUpdateToAll(const OrderBookEntry& update);

    void saveOrderBookGlobalBid(const OrderBookEntry& entry);
    void saveOrderBookGlobalAsk(const OrderBookEntry& entry);
    void saveOrderBookLocalBid(const OrderBookEntry& entry);
    void saveOrderBookLocalAsk(const OrderBookEntry& entry);

    double getGlobalBidOrderBookFirstPrice() const;
    double getGlobalAskOrderBookFirstPrice() const;
    double getLocalBidOrderBookFirstPrice() const;
    double getLocalAskOrderBookFirstPrice() const;
};
