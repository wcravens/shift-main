#include "OrderBook.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"

#include <algorithm>

#include <shift/miscutils/concurrency/Consumer.h>

/**
*   @brief  Constructs OrderBook instance with stock name.
*	@param	name: The stock name.
*/
OrderBook::OrderBook(const std::string& name)
    : m_symbol(name)
{
}

OrderBook::~OrderBook() /*override*/
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlag, m_cvOBEBuff, *m_th);
    m_th = nullptr;
}

/**
*   @brief  Thread-safely adds an OrderBookEntry to the buffer, then notify to process the order.
*	@param	entry: The OrderBookEntry to be enqueue the buffer.
*   @return nothing
*/
void OrderBook::enqueueOrderBook(OrderBookEntry&& entry)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxOBEBuff);
        m_obeBuff.push(std::move(entry));
    }
    m_cvOBEBuff.notify_one();
}

/**
*   @brief  Thread-safely save the OrderBookEntry items correspondingly with respect to their order book type.
*   @return nothing
*/
void OrderBook::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    while (true) {
        std::unique_lock<std::mutex> buffLock(m_mtxOBEBuff);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvOBEBuff, buffLock, [this] { return !m_obeBuff.empty(); }))
            return;

        switch (m_obeBuff.front().getType()) {
        case OrderBookEntry::Type::GLB_ASK:
            saveOrderBookGlobalAsk(m_obeBuff.front());
            break;
        case OrderBookEntry::Type::LOC_ASK:
            saveOrderBookLocalAsk(m_obeBuff.front());
            break;
        case OrderBookEntry::Type::GLB_BID:
            saveOrderBookGlobalBid(m_obeBuff.front());
            break;
        case OrderBookEntry::Type::LOC_BID:
            saveOrderBookLocalBid(m_obeBuff.front());
            break;
        default:
            break;
        }

        broadcastSingleUpdateToAll(m_obeBuff.front());
        m_obeBuff.pop();
    } // while
}

/**
*   @brief  Spawns and launch the process thread.
*   @return nothing
*/
void OrderBook::spawn()
{
    m_th.reset(new std::thread(&OrderBook::process, this));
}

/**
*   @brief  Record the target ID that subscribes this OrderBook and send the
*   @return nothing
*/
void OrderBook::onSubscribeOrderBook(const std::string& targetID)
{
    broadcastWholeOrderBookToOne(targetID);
    registerTarget(targetID);
}

/**
*   @brief  Thread-safely unregisters a user from the list by username.
*   @return nothing
*/
void OrderBook::onUnsubscribeOrderBook(const std::string& targetID)
{
    unregisterTarget(targetID);
}

/**
*   @brief  Thread-safely sends complete order books to one user user.
*   @return nothing
*/
void OrderBook::broadcastWholeOrderBookToOne(const std::string& targetID)
{
    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    const std::vector<std::string> targetList{ targetID };

    std::lock_guard<std::mutex> guard_A(m_mtxOdrBkGlobalAsk);
    std::lock_guard<std::mutex> guard_a(m_mtxOdrBkLocalAsk);
    std::lock_guard<std::mutex> guard_B(m_mtxOdrBkGlobalBid);
    std::lock_guard<std::mutex> guard_b(m_mtxOdrBkLocalBid);

    if (!m_odrBkGlobalAsk.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkGlobalAsk);
    if (!m_odrBkLocalAsk.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkLocalAsk);
    if (!m_odrBkGlobalBid.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkGlobalBid);
    if (!m_odrBkLocalBid.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely sends complete order books to all targets.
*   @return nothing
*/
void OrderBook::broadcastWholeOrderBookToAll()
{
    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    auto targetList = getTargetList();
    if (targetList.empty())
        return;

    std::lock_guard<std::mutex> guard_A(m_mtxOdrBkGlobalAsk);
    std::lock_guard<std::mutex> guard_a(m_mtxOdrBkLocalAsk);
    std::lock_guard<std::mutex> guard_B(m_mtxOdrBkGlobalBid);
    std::lock_guard<std::mutex> guard_b(m_mtxOdrBkLocalBid);

    if (!m_odrBkGlobalAsk.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkGlobalAsk);
    if (!m_odrBkLocalAsk.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkLocalAsk);
    if (!m_odrBkGlobalBid.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkGlobalBid);
    if (!m_odrBkLocalBid.empty())
        toWCPtr->sendOrderBook(targetList, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely sends an order book's single update to all targets.
*	@param	update: The order book update record to be sent.
*   @return nothing
*/
void OrderBook::broadcastSingleUpdateToAll(const OrderBookEntry& update)
{
    auto targetList = getTargetList();
    if (targetList.empty())
        return;

    using ulock_t = std::unique_lock<std::mutex>;
    ulock_t lock;

    switch (update.getType()) {
    case OrderBookEntry::Type::GLB_ASK: {
        lock = ulock_t(m_mtxOdrBkGlobalAsk);
        break;
    }
    case OrderBookEntry::Type::LOC_ASK: {
        lock = ulock_t(m_mtxOdrBkLocalAsk);
        break;
    }
    case OrderBookEntry::Type::GLB_BID: {
        lock = ulock_t(m_mtxOdrBkGlobalBid);
        break;
    }
    case OrderBookEntry::Type::LOC_BID: {
        lock = ulock_t(m_mtxOdrBkLocalBid);
        break;
    }
    default:
        return;
    }

    FIXAcceptor::getInstance()->sendOrderBookUpdate(targetList, update);
}

/**
*   @brief  Thread-safely saves the latest order book entry to global bid order book, as the ceiling of all hitherto bid prices.
*/
void OrderBook::saveOrderBookGlobalBid(const OrderBookEntry& entry)
{
    double price = entry.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalBid);

    m_odrBkGlobalBid[price][entry.getDestination()] = entry;
    // discard all higher bid prices, if any:
    m_odrBkGlobalBid.erase(m_odrBkGlobalBid.upper_bound(price), m_odrBkGlobalBid.end());
}

/**
*   @brief  Thread-safely saves the latest order book entry to global ask order book, as the bottom of all hitherto ask prices.
*/
void OrderBook::saveOrderBookGlobalAsk(const OrderBookEntry& entry)
{
    double price = entry.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalAsk);

    m_odrBkGlobalAsk[price][entry.getDestination()] = entry;
    // discard all lower ask prices, if any:
    m_odrBkGlobalAsk.erase(m_odrBkGlobalAsk.begin(), m_odrBkGlobalAsk.lower_bound(price));
}

/**
*   @brief  Thread-safely saves order book entry to local bid order book at specific price.
*   @param	entry: The order book entry to be saved.
*   @return nothing
*/
inline void OrderBook::saveOrderBookLocalBid(const OrderBookEntry& entry)
{
    s_saveOdrBkLocal(entry, m_mtxOdrBkLocalBid, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely saves order book entry to local ask order book at specific price.
*   @param	entry: The order book entry to be saved.
*   @return nothing
*/
inline void OrderBook::saveOrderBookLocalAsk(const OrderBookEntry& entry)
{
    s_saveOdrBkLocal(entry, m_mtxOdrBkLocalAsk, m_odrBkLocalAsk);
}

/*static*/ void OrderBook::s_saveOdrBkLocal(const OrderBookEntry& entry, std::mutex& mtxOdrBkLocal, std::map<double, std::map<std::string, OrderBookEntry>>& odrBkLocal)
{
    double price = entry.getPrice();
    std::lock_guard<std::mutex> guard(mtxOdrBkLocal);

    if (entry.getSize() > 0) {
        odrBkLocal[price][entry.getDestination()] = entry;
    } else {
        odrBkLocal[price].erase(entry.getDestination());
        if (odrBkLocal[price].empty()) {
            odrBkLocal.erase(price);
        }
    }
}

double OrderBook::getGlobalBidOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalBid);
    if (!m_odrBkGlobalBid.empty())
        return (--m_odrBkGlobalBid.end())->first;
    else
        return 0.0;
}

double OrderBook::getGlobalAskOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalAsk);
    if (!m_odrBkGlobalAsk.empty())
        return m_odrBkGlobalAsk.begin()->first;
    else
        return 0.0;
}

double OrderBook::getLocalBidOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkLocalBid);
    if (!m_odrBkLocalBid.empty())
        return (--m_odrBkLocalBid.end())->first;
    else
        return 0.0;
}

double OrderBook::getLocalAskOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkLocalAsk);
    if (!m_odrBkLocalAsk.empty())
        return m_odrBkLocalAsk.begin()->first;
    else
        return 0.0;
}
