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
public:
    OrderBook() = default;
    OrderBook(const std::string& name);
    ~OrderBook() override;

    void enqueueOrderBookUpdate(OrderBookEntry&& update);
    void process();
    void spawn();
    // void stop();

    void onSubscribeOrderBook(const std::string& userID);
    void onUnsubscribeOrderBook(const std::string& userID);

    void broadcastWholeOrderBookToOne(const std::string& userID);
    void broadcastWholeOrderBookToAll();
    void broadcastSingleUpdateToAll(const OrderBookEntry& update);

    void saveGlobalBidOrderBookUpdate(const OrderBookEntry& update);
    void saveGlobalAskOrderBookUpdate(const OrderBookEntry& update);
    void saveLocalBidOrderBookUpdate(const OrderBookEntry& update);
    void saveLocalAskOrderBookUpdate(const OrderBookEntry& update);
    static void s_saveLocalOrderBookUpdate(const OrderBookEntry& update, std::mutex& mtxLocalOrderBookk, std::map<double, std::map<std::string, OrderBookEntry>>& localOrderBook);

    double getGlobalBidOrderBookFirstPrice() const;
    double getGlobalAskOrderBookFirstPrice() const;
    double getLocalBidOrderBookFirstPrice() const;
    double getLocalAskOrderBookFirstPrice() const;

private:
    std::string m_symbol; ///> The stock name of this OrderBook instance.

    mutable std::mutex m_mtxOBEBuff; ///> Mutex for m_obeBuff.
    mutable std::mutex m_mtxGlobalBidOrderBook;
    mutable std::mutex m_mtxGlobalAskOrderBook;
    mutable std::mutex m_mtxLocalBidOrderBook;
    mutable std::mutex m_mtxLocalAskOrderBook;

    std::unique_ptr<std::thread> m_th;
    std::condition_variable m_cvOBEBuff;
    std::promise<void> m_quitFlag;

    std::map<double, std::map<std::string, OrderBookEntry>> m_globalBidOrderBook; // price, destination, order book entry
    std::map<double, std::map<std::string, OrderBookEntry>> m_globalAskOrderBook;
    std::map<double, std::map<std::string, OrderBookEntry>> m_localBidOrderBook;
    std::map<double, std::map<std::string, OrderBookEntry>> m_localAskOrderBook;

    std::queue<OrderBookEntry> m_obeBuff;
};
