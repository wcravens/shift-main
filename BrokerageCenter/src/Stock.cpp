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
*   @brief  Thread-safely adds an OrderBook to the orderbook buffer, then notify to process the orders.
*	@param	ob: The OrderBook to be enqueue the buffer.
*   @return nothing
*/
void Stock::enqueueOrderBook(const OrderBookEntry& ob)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxOdrBkBuff);
        m_odrBkBuff.push(ob);
    }
    m_cvOdrBkBuff.notify_one();
}

/**
*   @brief  Thread-safely save the OrderBook items correspondingly with respect to their order type.
*   @return nothing
*/
void Stock::process()
{
    thread_local auto quitFut = m_quitFlag.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxStock);
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
*   @brief  Wait for the process thread to finish for at most 5 seconds.
*   @return nothing
*/
void Stock::stop()
{
    std::lock_guard<std::mutex> guard(m_mtxStock);
    m_flag = true;
    m_th->detach();
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
        m_userList.insert(userName);
    }
    BCDocuments::getInstance()->addOrderbookSymbolToUser(userName, m_symbol);
}

/**
*   @brief  Thread-safely unregisters a user from the list by user name.
*	@param	userName: The user name of the user.
*   @return nothing
*/
void Stock::unregisterUserInStock(const std::string& userName)
{
    std::lock_guard<std::mutex> guard(m_mtxStockUserList);
    m_userList.erase(userName);
}

/**
*   @brief  Thread-safely sends an orderbook's single update to all users.
*	@param	record: The OrderBook record to be sent.
*   @return nothing
*/
void Stock::broadcastSingleUpdateToAll(const OrderBookEntry& record)
{
    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    using OBT = OrderBookEntry::ORDER_BOOK_TYPE;

    using ulock_t = std::unique_lock<std::mutex>;
    ulock_t lock;

    switch (record.getType()) {
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

    toWCPtr->sendOrderbookUpdate2All(m_userList, record);
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
        toWCPtr->sendOrderBook(userName, m_odrBkGlobalAsk);
    if (!m_odrBkLocalAsk.empty())
        toWCPtr->sendOrderBook(userName, m_odrBkLocalAsk);
    if (!m_odrBkGlobalBid.empty())
        toWCPtr->sendOrderBook(userName, m_odrBkGlobalBid);
    if (!m_odrBkLocalBid.empty())
        toWCPtr->sendOrderBook(userName, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely sends complete order books to all users.
*   @return nothing
*/
void Stock::broadcastWholeOrderBookToAll()
{
    std::lock_guard<std::mutex> guard(m_mtxStockUserList);
    if (m_userList.empty())
        return; // nobody to sent to.

    std::lock_guard<std::mutex> guard_A(m_mtxOdrBkGlobalAsk);
    std::lock_guard<std::mutex> guard_a(m_mtxOdrBkLocalAsk);
    std::lock_guard<std::mutex> guard_B(m_mtxOdrBkGlobalBid);
    std::lock_guard<std::mutex> guard_b(m_mtxOdrBkLocalBid);

    FIXAcceptor* toWCPtr = FIXAcceptor::getInstance();
    if (!m_odrBkGlobalAsk.empty())
        toWCPtr->sendNewBook2all(m_userList, m_odrBkGlobalAsk);
    if (!m_odrBkLocalAsk.empty())
        toWCPtr->sendNewBook2all(m_userList, m_odrBkLocalAsk);
    if (!m_odrBkGlobalBid.empty())
        toWCPtr->sendNewBook2all(m_userList, m_odrBkGlobalBid);
    if (!m_odrBkLocalBid.empty())
        toWCPtr->sendNewBook2all(m_userList, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely saves the latest orderbook record to Bid orderbook, as the ceiling of all hitherto bid prices.
*   @param	record: The orderbook record to be saved.
*   @return nothing
*/
void Stock::saveOrderBookGlobalBid(const OrderBookEntry& record)
{
    double price = record.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalBid);

    m_odrBkGlobalBid[price][record.getDestination()] = record;
    // discard all higher bid prices, if any:
    m_odrBkGlobalBid.erase(m_odrBkGlobalBid.upper_bound(price), m_odrBkGlobalBid.end());
}

/**
*   @brief  Thread-safely saves the latest orderbook record to Ask orderbook, as the bottom of all hitherto ask prices.
*   @param	record: The orderbook record to be saved.
*   @return nothing
*/
void Stock::saveOrderBookGlobalAsk(const OrderBookEntry& record)
{
    double price = record.getPrice();
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalAsk);

    m_odrBkGlobalAsk[price][record.getDestination()] = record;
    // discard all lower ask prices, if any:
    m_odrBkGlobalAsk.erase(m_odrBkGlobalAsk.begin(), m_odrBkGlobalAsk.lower_bound(price));
}

static void s_saveOdrBkLocal(const OrderBookEntry& record, std::mutex& mtxOdrBkLocal, std::map<double, std::map<std::string, OrderBookEntry>>& odrBkLocal)
{
    double price = record.getPrice();
    std::lock_guard<std::mutex> guard(mtxOdrBkLocal);

    if (record.getSize() > 0) {
        odrBkLocal[price][record.getDestination()] = record;
    } else {
        odrBkLocal[price].erase(record.getDestination());
        if (odrBkLocal[price].empty()) {
            odrBkLocal.erase(price);
        }
    }
}

/**
*   @brief  Thread-safely saves orderbook record to bidOrderBook at specific price.
*   @param	record: The orderbook record to be saved.
*   @return nothing
*/
inline void Stock::saveOrderBookLocalBid(const OrderBookEntry& record)
{
    ::s_saveOdrBkLocal(record, m_mtxOdrBkLocalBid, m_odrBkLocalBid);
}

/**
*   @brief  Thread-safely saves orderbook record to askOrderBook at specific price.
*   @param	record: The orderbook record to be saved.
*   @return nothing
*/
inline void Stock::saveOrderBookLocalAsk(const OrderBookEntry& record)
{
    ::s_saveOdrBkLocal(record, m_mtxOdrBkLocalAsk, m_odrBkLocalAsk);
}

double Stock::getGlobalBidOrderbookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalBid);
    if (!m_odrBkGlobalBid.empty())
        return (--m_odrBkGlobalBid.end())->first;
    else
        return 0.0;
}

double Stock::getGlobalAskOrderbookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkGlobalAsk);
    if (!m_odrBkGlobalAsk.empty())
        return m_odrBkGlobalAsk.begin()->first;
    else
        return 0.0;
}

double Stock::getLocalBidOrderbookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkLocalBid);
    if (!m_odrBkLocalBid.empty())
        return (--m_odrBkLocalBid.end())->first;
    else
        return 0.0;
}

double Stock::getLocalAskOrderbookFirstPrice() const
{
    std::lock_guard<std::mutex> guard(m_mtxOdrBkLocalAsk);
    if (!m_odrBkLocalAsk.empty())
        return m_odrBkLocalAsk.begin()->first;
    else
        return 0.0;
}
