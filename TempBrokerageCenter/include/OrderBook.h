#pragma once

#include "Interfaces.h"
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
class OrderBook : public ITargetsInfo {
private:
    std::string m_symbol; ///> The stock name of this OrderBook instance.

    mutable std::mutex m_mtxOBEBuff; ///> Mutex for m_obeBuff.
    mutable std::mutex m_mtxOdrBkGlobalAsk;
    mutable std::mutex m_mtxOdrBkGlobalBid;
    mutable std::mutex m_mtxOdrBkLocalAsk;
    mutable std::mutex m_mtxOdrBkLocalBid;

    std::unique_ptr<std::thread> m_th;
    std::condition_variable m_cvOBEBuff;
    std::promise<void> m_quitFlag;

    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkGlobalAsk; // price, destination, order book entry
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkLocalAsk;
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkGlobalBid;
    std::map<double, std::map<std::string, OrderBookEntry>> m_odrBkLocalBid;

    std::queue<OrderBookEntry> m_obeBuff;

public:
    OrderBook();
    OrderBook(std::string name);
    ~OrderBook();

    void enqueueOrderBook(const OrderBookEntry& entry);
    void process();
    void spawn();
    // void stop();

    void onSubscribeOrderBook(const std::string& username);
    void onUnsubscribeInOrderBook(const std::string& username);

    void broadcastWholeOrderBookToOne(const std::string& username);
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
