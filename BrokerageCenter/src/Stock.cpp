#include "Stock.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"

#include <shift/miscutils/concurrency/Consumer.h>

/**
*   @brief  Default constructs an Stock instance.
*/
Stock::Stock() = default;

/**
*   @brief  Constructs Stock instance with stock name.
*	@param	name: The stock name.
*/
Stock::Stock(std::string name)
    : m_symbol(std::move(name))
{
}

Stock::~Stock()
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlag, m_cvOdrBkBuff, *m_th);
    m_th = nullptr;
}

/**
*   @brief  Thread-safely adds an OrderBookEntry to the buffer, then notify to process the order.
*	@param	entry: The OrderBookEntry to be enqueue the buffer.
*   @return nothing
*/
void Stock::enqueueOrderBook(const OrderBookEntry& entry)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxOdrBkBuff);
        m_odrBkBuff.push(entry);
    }
    m_cvOdrBkBuff.notify_one();
}

/**
*   @brief  Thread-safely save the OrderBookEntry items correspondingly with respect to their order book type.
*   @return nothing
*/
void Stock::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    while (true) {
        std::unique_lock<std::mutex> buffLock(m_mtxOdrBkBuff);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvOdrBkBuff, buffLock, [this] { return !m_odrBkBuff.empty(); }))
            return;

        using OBT = OrderBookEntry::ORDER_BOOK_TYPE;

        switch (m_odrBkBuff.front().getType()) {
        case OBT::GLB_ASK:
            saveOrderBookGlobalAsk(m_odrBkBuff.front());
            break;
        case OBT::LOC_ASK:
            saveOrderBookLocalAsk(m_odrBkBuff.front());
            break;
        case OBT::GLB_BID:
            saveOrderBookGlobalBid(m_odrBkBuff.front());
            break;
        case OBT::LOC_BID:
            saveOrderBookLocalBid(m_odrBkBuff.front());
            break;
        default:
            break;
        }

        broadcastSingleUpdateToAll(m_odrBkBuff.front());
        m_odrBkBuff.pop();
    } // while
}

/**
*   @brief  Spawns and launch the process thread.
*   @return nothing
*/
void Stock::spawn()
{
    m_th.reset(new std::thread(&Stock::process, this));
}

/**
*   @brief  Thread-safely registers a user to the list by user name.
*	@param	userName: The name of the user to be registered.
*   @return nothing
*/
void Stock::registerUserInStock(const std::string& userName)
{
    broadcastWholeOrderBookToOne(userName);
    {
        std::lock_guard<std::mutex> guard(m_mtxStockUserList);
        m_stockUserList.insert(userName);
    }
    BCDocuments::getInstance()->addOrderBookSymbolToUser(userName, m_symbol);
}

/**
*   @brief  Thread-safely unregisters a user from the list by user name.
*	@param	userName: The user name of the user.
*   @return nothing
*/
void Stock::unregisterUserInStock(const std::string& userName)
{
    std::lock_guard<std::mutex> guard(m_mtxStockUserList);
    m_stockUserList.erase(userName);
}

/**
*   @brief  Thread-safely sends an order book's single update to all users.
*	@param	entry: The order book entry record to be sent.
*   @return nothing
*/
void Stock::broadcastSingleUpdateToAll(const OrderBookEntry& entry)
{
    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    using OBT = OrderBookEntry::ORDER_BOOK_TYPE;

    std::lock_guard<std::mutex> guard(m_mtxStockUserList);

    using ulock_t = std::unique_lock<std::mutex>;
    ulock_t lock;

    switch (entry.getType()) {
    case OBT::GLB_ASK: {
        lock = ulock_t(m_mtxOdrBkGlobalAsk);
        break;
    }
    case OBT::LOC_ASK: {
        lock = ulock_t(m_mtxOdrBkLocalAsk);
        break;
    }
    case OBT::GLB_BID: {
        lock = ulock_t(m_mtxOdrBkGlobalBid);
        break;
    }
    case OBT::LOC_BID: {
        lock = ulock_t(m_mtxOdrBkLocalBid);
        break;
    }
    default:
        return;
    }

    toWCPtr->sendOrderBookUpdate(m_stockUserList, entry);
}

/**
*   @brief  Thread-safely sends complete order books to one user user.
*	@param	userName: The user user to be sent to.
*   @return nothing
*/
void Stock::broadcastWholeOrderBookToOne(const std::string& userName)
{
    std::lock_guard<std::mutex> guard_A(m_mtxOdrBkGlobalAsk);
    std::lock_guard<std::mutex> guard_a(m_mtxOdrBkLocalAsk);
    std::lock_guard<std::mutex> guard_B(m_mtxOdrBkGlobalBid);
    std::lock_guard<std::mutex> guard_b(m_mtxOdrBkLocalBid);

    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    if (!m_odrBkGlobalAsk.empty())
        toWCPtr->sendOrderBook({ userName }, m_odrBkGlobalAsk);
    if (!m_odrBkLocalAsk.empty())
        toWCPtr->sendOrderBook({ userName }, m_odrBkLocalAsk);
    if (!m_odrBkGlobalBid.empty())
        toWCPtr->sendOrderBook({ userName }, m_odrBkGlobalBid);
    if (!m_odrBkLocalBid.empty())
        toWCPtr->sendOrderBook({ userName }, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely sends complete order books to all users.
*   @return nothing
*/
void Stock::broadcastWholeOrderBookToAll()
{
    std::lock_guard<std::mutex> guard(m_mtxStockUserList);
    if (m_stockUserList.empty())
        return; // nobody to sent to.

    std::lock_guard<std::mutex> guard_A(m_mtxOdrBkGlobalAsk);
    std::lock_guard<std::mutex> guard_a(m_mtxOdrBkLocalAsk);
    std::lock_guard<std::mutex> guard_B(m_mtxOdrBkGlobalBid);
    std::lock_guard<std::mutex> guard_b(m_mtxOdrBkLocalBid);

    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    if (!m_odrBkGlobalAsk.empty())
        toWCPtr->sendOrderBook(m_stockUserList, m_odrBkGlobalAsk);
    if (!m_odrBkLocalAsk.empty())
        toWCPtr->sendOrderBook(m_stockUserList, m_odrBkLocalAsk);
    if (!m_odrBkGlobalBid.empty())
        toWCPtr->sendOrderBook(m_stockUserList, m_odrBkGlobalBid);
    if (!m_odrBkLocalBid.empty())
        toWCPtr->sendOrderBook(m_stockUserList, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely saves the latest order book entry to global bid order book, as the ceiling of all hitherto bid prices.
*   @param	entry: The order book entry to be saved.
*   @return nothing
*/
void Stock::saveOrderBookGlobalBid(const OrderBookEntry& entry)
{
    double price = entry.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalBid);

    m_odrBkGlobalBid[price][entry.getDestination()] = entry;
    // discard all higher bid prices, if any:
    m_odrBkGlobalBid.erase(m_odrBkGlobalBid.upper_bound(price), m_odrBkGlobalBid.end());
}

/**
*   @brief  Thread-safely saves the latest order book entry to global ask order book, as the bottom of all hitherto ask prices.
*   @param	entry: The order book entry to be saved.
*   @return nothing
*/
void Stock::saveOrderBookGlobalAsk(const OrderBookEntry& entry)
{
    double price = entry.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalAsk);

    m_odrBkGlobalAsk[price][entry.getDestination()] = entry;
    // discard all lower ask prices, if any:
    m_odrBkGlobalAsk.erase(m_odrBkGlobalAsk.begin(), m_odrBkGlobalAsk.lower_bound(price));
}

static void s_saveOdrBkLocal(const OrderBookEntry& entry, std::mutex& mtxOdrBkLocal, std::map<double, std::map<std::string, OrderBookEntry>>& odrBkLocal)
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

/**
*   @brief  Thread-safely saves order book entry to local bid order book at specific price.
*   @param	entry: The order book entry to be saved.
*   @return nothing
*/
inline void Stock::saveOrderBookLocalBid(const OrderBookEntry& entry)
{
    ::s_saveOdrBkLocal(entry, m_mtxOdrBkLocalBid, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely saves order book entry to local ask order book at specific price.
*   @param	entry: The order book entry to be saved.
*   @return nothing
*/
inline void Stock::saveOrderBookLocalAsk(const OrderBookEntry& entry)
{
    ::s_saveOdrBkLocal(entry, m_mtxOdrBkLocalAsk, m_odrBkLocalAsk);
}

double Stock::getGlobalBidOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalBid);
    if (!m_odrBkGlobalBid.empty())
        return (--m_odrBkGlobalBid.end())->first;
    else
        return 0.0;
}

double Stock::getGlobalAskOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalAsk);
    if (!m_odrBkGlobalAsk.empty())
        return m_odrBkGlobalAsk.begin()->first;
    else
        return 0.0;
}

double Stock::getLocalBidOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkLocalBid);
    if (!m_odrBkLocalBid.empty())
        return (--m_odrBkLocalBid.end())->first;
    else
        return 0.0;
}

double Stock::getLocalAskOrderBookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkLocalAsk);
    if (!m_odrBkLocalAsk.empty())
        return m_odrBkLocalAsk.begin()->first;
    else
        return 0.0;
}
