#include "OrderBook.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"

#include <algorithm>

#include <shift/miscutils/concurrency/Consumer.h>

/**
 * @brief Constructs OrderBook instance with stock name.
 * @param name The stock name.
 */
OrderBook::OrderBook(const std::string& name)
    : m_symbol(name)
{
}

OrderBook::~OrderBook() // override
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlag, m_cvOBEBuff, *m_th);
    m_th = nullptr;
}

/**
* @brief Thread-safely adds an OrderBookEntry to the buffer, then notify to process the order.
* @param update The OrderBookEntry to be enqueue the buffer.
*/
void OrderBook::enqueueOrderBookUpdate(OrderBookEntry&& update)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxOBEBuff);
        m_obeBuff.push(std::move(update));
    }
    m_cvOBEBuff.notify_one();
}

/**
 * @brief Thread-safely save the OrderBookEntry items correspondingly with respect to their order book type.
 */
void OrderBook::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    while (true) {
        std::unique_lock<std::mutex> buffLock(m_mtxOBEBuff);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvOBEBuff, buffLock, [this] { return !m_obeBuff.empty(); }))
            return;

        switch (m_obeBuff.front().getType()) {
        case OrderBookEntry::Type::GLB_BID:
            saveGlobalBidOrderBookUpdate(m_obeBuff.front());
            break;
        case OrderBookEntry::Type::GLB_ASK:
            saveGlobalAskOrderBookUpdate(m_obeBuff.front());
            break;
        case OrderBookEntry::Type::LOC_BID:
            saveLocalBidOrderBookUpdate(m_obeBuff.front());
            break;
        case OrderBookEntry::Type::LOC_ASK:
            saveLocalAskOrderBookUpdate(m_obeBuff.front());
            break;
        default:
            break;
        }

        broadcastSingleUpdateToAll(m_obeBuff.front());
        m_obeBuff.pop();
    } // while
}

/**
 * @brief Spawns and launch the process thread.
 */
void OrderBook::spawn()
{
    m_th.reset(new std::thread(&OrderBook::process, this));
}

/**
 * @brief Record the target ID that subscribes to this order book and send a copy of the order book to it.
 */
void OrderBook::onSubscribeOrderBook(const std::string& targetID)
{
    broadcastWholeOrderBookToOne(targetID);
    registerTarget(targetID);
}

/**
 * @brief Thread-safely unregisters a target.
 */
void OrderBook::onUnsubscribeOrderBook(const std::string& targetID)
{
    unregisterTarget(targetID);
}

/**
 * @brief Thread-safely sends complete order books to one user user.
 */
void OrderBook::broadcastWholeOrderBookToOne(const std::string& targetID)
{
    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    const std::vector<std::string> targetList{ targetID };

    std::lock_guard<std::mutex> guard_B(m_mtxGlobalBidOrderBook);
    std::lock_guard<std::mutex> guard_A(m_mtxGlobalAskOrderBook);
    std::lock_guard<std::mutex> guard_b(m_mtxLocalBidOrderBook);
    std::lock_guard<std::mutex> guard_a(m_mtxLocalAskOrderBook);

    if (!m_globalBidOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_globalBidOrderBook);
    if (!m_globalAskOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_globalAskOrderBook);
    if (!m_localBidOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_localBidOrderBook);
    if (!m_localAskOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_localAskOrderBook);
}

/**
 * @brief Thread-safely sends complete order books to all targets.
 */
void OrderBook::broadcastWholeOrderBookToAll()
{
    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    auto targetList = getTargetList();
    if (targetList.empty())
        return;

    std::lock_guard<std::mutex> guard_B(m_mtxGlobalBidOrderBook);
    std::lock_guard<std::mutex> guard_A(m_mtxGlobalAskOrderBook);
    std::lock_guard<std::mutex> guard_b(m_mtxLocalBidOrderBook);
    std::lock_guard<std::mutex> guard_a(m_mtxLocalAskOrderBook);

    if (!m_globalBidOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_globalBidOrderBook);
    if (!m_globalAskOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_globalAskOrderBook);
    if (!m_localBidOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_localBidOrderBook);
    if (!m_localAskOrderBook.empty())
        toWCPtr->sendOrderBook(targetList, m_localAskOrderBook);
}

/**
 * @brief Thread-safely sends an order book's single update to all targets.
 * @param update The order book update record to be sent.
 */
void OrderBook::broadcastSingleUpdateToAll(const OrderBookEntry& update)
{
    auto targetList = getTargetList();
    if (targetList.empty())
        return;

    using ulock_t = std::unique_lock<std::mutex>;
    ulock_t lock;

    switch (update.getType()) {
    case OrderBookEntry::Type::GLB_BID: {
        lock = ulock_t(m_mtxGlobalBidOrderBook);
        break;
    }
    case OrderBookEntry::Type::GLB_ASK: {
        lock = ulock_t(m_mtxGlobalAskOrderBook);
        break;
    }
    case OrderBookEntry::Type::LOC_BID: {
        lock = ulock_t(m_mtxLocalBidOrderBook);
        break;
    }
    case OrderBookEntry::Type::LOC_ASK: {
        lock = ulock_t(m_mtxLocalAskOrderBook);
        break;
    }
    default:
        return;
    }

    FIXAcceptor::getInstance()->sendOrderBookUpdate(targetList, update);
}

/**
 * @brief Thread-safely saves the latest order book entry to global bid order book, as the ceiling of all hitherto bid prices.
 */
void OrderBook::saveGlobalBidOrderBookUpdate(const OrderBookEntry& update)
{
    double price = update.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxGlobalBidOrderBook);

    // price <= 0.0 means clear the order book
    if (price <= 0.0) {
        m_globalBidOrderBook.clear();
        return;
    }

    m_globalBidOrderBook[price][update.getDestination()] = update;

    // discard all higher bid prices, if any
    m_globalBidOrderBook.erase(m_globalBidOrderBook.upper_bound(price), m_globalBidOrderBook.end());
}

/**
 * @brief Thread-safely saves the latest order book entry to global ask order book, as the bottom of all hitherto ask prices.
 */
void OrderBook::saveGlobalAskOrderBookUpdate(const OrderBookEntry& update)
{
    double price = update.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxGlobalAskOrderBook);

    // price <= 0.0 means clear the order book
    if (price <= 0.0) {
        m_globalAskOrderBook.clear();
        return;
    }

    m_globalAskOrderBook[price][update.getDestination()] = update;

    // discard all lower ask prices, if any
    m_globalAskOrderBook.erase(m_globalAskOrderBook.begin(), m_globalAskOrderBook.lower_bound(price));
}

/**
 * @brief Thread-safely saves order book entry to local bid order book at specific price.
 * @param update The order book entry to be saved.
 */
inline void OrderBook::saveLocalBidOrderBookUpdate(const OrderBookEntry& update)
{
    s_saveLocalOrderBookUpdate(update, m_mtxLocalBidOrderBook, m_localBidOrderBook);
}

/**
 * @brief Thread-safely saves order book entry to local ask order book at specific price.
 * @param update The order book entry to be saved.
 */
inline void OrderBook::saveLocalAskOrderBookUpdate(const OrderBookEntry& update)
{
    s_saveLocalOrderBookUpdate(update, m_mtxLocalAskOrderBook, m_localAskOrderBook);
}

/*static*/ void OrderBook::s_saveLocalOrderBookUpdate(const OrderBookEntry& update, std::mutex& mtxLocalOrderBookk, std::map<double, std::map<std::string, OrderBookEntry>>& localOrderBook)
{
    double price = update.getPrice();
    std::lock_guard<std::mutex> guard(mtxLocalOrderBookk);

    // price <= 0.0 means clear the order book
    if (price <= 0.0) {
        localOrderBook.clear();
        return;
    }

    if (update.getSize() > 0) {
        localOrderBook[price][update.getDestination()] = update;
    } else {
        localOrderBook[price].erase(update.getDestination());
        if (localOrderBook[price].empty()) {
            localOrderBook.erase(price);
        }
    }
}

double OrderBook::getGlobalBidOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxGlobalBidOrderBook);
    if (!m_globalBidOrderBook.empty())
        return (--m_globalBidOrderBook.end())->first;
    else
        return 0.0;
}

double OrderBook::getGlobalAskOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxGlobalAskOrderBook);
    if (!m_globalAskOrderBook.empty())
        return m_globalAskOrderBook.begin()->first;
    else
        return 0.0;
}

double OrderBook::getLocalBidOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxLocalBidOrderBook);
    if (!m_localBidOrderBook.empty())
        return (--m_localBidOrderBook.end())->first;
    else
        return 0.0;
}

double OrderBook::getLocalAskOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxLocalAskOrderBook);
    if (!m_localAskOrderBook.empty())
        return m_localAskOrderBook.begin()->first;
    else
        return 0.0;
}
