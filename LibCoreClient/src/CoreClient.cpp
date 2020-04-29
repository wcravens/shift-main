#include "CoreClient.h"

#include "BestPrice.h"
#include "FIXInitiator.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
#include <terminal/Common.h>
#else
#include <shift/miscutils/terminal/Common.h>
#endif

using namespace std::chrono_literals;

namespace shift {

CoreClient::CoreClient(std::string username)
    : m_fixInitiator { nullptr }
    , m_username { std::move(username) }
    , m_verbose { false }
    , m_submittedOrdersSize { 0 }
    , m_waitingListSize { 0 }
{
}

void CoreClient::setVerbose(bool verbose)
{
    m_verbose = verbose;
}

auto CoreClient::isVerbose() const -> bool
{
    return m_verbose;
}

auto CoreClient::isConnected() const -> bool
{
    return (m_fixInitiator != nullptr && m_fixInitiator->isConnected());
}

auto CoreClient::getUsername() const -> const std::string&
{
    return m_username;
}

void CoreClient::setUsername(const std::string& username)
{
    m_username = username;
}

auto CoreClient::getUserID() const -> const std::string&
{
    return m_userID;
}

void CoreClient::setUserID(const std::string& userID)
{
    m_userID = userID;
}

auto CoreClient::getAttachedClients() -> std::vector<CoreClient*>
{
    if (!isConnected()) {
        return std::vector<CoreClient*>();
    }

    return m_fixInitiator->getAttachedClients();
}

/**
 * @brief Method to submit orders from Core Client to Brokerage Center.
 * @param order as a Order object contains all required information.
 */
void CoreClient::submitOrder(const Order& order)
{
    if (!isConnected()) {
        return;
    }

    if (order.getType() != Order::Type::CANCEL_BID && order.getType() != Order::Type::CANCEL_ASK) {
        std::lock_guard<std::mutex> lk(m_mutex_orders);
        m_submittedOrdersIDs.push_back(order.getID());
        m_submittedOrders[order.getID()] = order;
        ++m_submittedOrdersSize;
    }

    return m_fixInitiator->submitOrder(order, getUserID());
}

/**
 * @brief Method to submit cancel orders from Core Client to Brokerage Center.
 * @param order as a Order object contains all required information.
 */
void CoreClient::submitCancellation(Order order)
{
    if (!isConnected()) {
        return;
    }

    if ((order.getType() == Order::Type::LIMIT_BUY) || (order.getType() == Order::Type::MARKET_BUY)) {
        order.setType(Order::Type::CANCEL_BID);
    } else if ((order.getType() == Order::Type::LIMIT_SELL) || (order.getType() == Order::Type::MARKET_SELL)) {
        order.setType(Order::Type::CANCEL_ASK);
    }

    order.setSize(order.getSize() - order.getExecutedSize());

    return m_fixInitiator->submitOrder(order, getUserID());
}

auto CoreClient::getPortfolioSummary() -> PortfolioSummary
{
    std::lock_guard<std::mutex> lk(m_mutex_portfolioSummary);
    return m_portfolioSummary;
}

auto CoreClient::getPortfolioItems() -> std::map<std::string, PortfolioItem>
{
    std::lock_guard<std::mutex> lock(m_mutex_symbol_portfolioItem);
    return m_symbol_portfolioItem;
}

auto CoreClient::getPortfolioItem(const std::string& symbol) -> PortfolioItem
{
    std::lock_guard<std::mutex> lock(m_mutex_symbol_portfolioItem);
    return m_symbol_portfolioItem[symbol];
}

auto CoreClient::getUnrealizedPL(const std::string& symbol /* = "" */) -> double
{
    if (!isConnected()) {
        return 0.0;
    }

    double unrealizedPL = 0.0;

    if (!symbol.empty()) {
        auto portfolioItem = getPortfolioItem(symbol);
        unrealizedPL = (getClosePrice(symbol) - portfolioItem.getPrice()) * portfolioItem.getShares();
    } else {
        for (const auto& [s, portfolioItem] : getPortfolioItems()) {
            unrealizedPL += (getClosePrice(s) - portfolioItem.getPrice()) * portfolioItem.getShares();
        }
    }

    return unrealizedPL;
}

auto CoreClient::getSubmittedOrdersSize() -> int
{
    return m_submittedOrdersSize;
}

auto CoreClient::getSubmittedOrders() -> std::vector<Order>
{
    std::vector<Order> submittedOrders;

    std::lock_guard<std::mutex> lk(m_mutex_orders);
    for (const std::string& id : m_submittedOrdersIDs) {
        submittedOrders.push_back(m_submittedOrders[id]);
    }

    return submittedOrders;
}

auto CoreClient::getOrder(const std::string& orderID) -> Order
{
    std::lock_guard<std::mutex> lk(m_mutex_orders);
    return m_submittedOrders[orderID];
}

auto CoreClient::getExecutedOrders(const std::string& orderID) -> std::vector<Order>
{
    std::vector<Order> executedOrders;

    std::lock_guard<std::mutex> lk(m_mutex_orders);
    auto range = m_executedOrders.equal_range(orderID);
    for (auto it = range.first; it != range.second; ++it) {
        executedOrders.push_back(it->second);
    }

    return executedOrders;
}

auto CoreClient::getWaitingListSize() -> int
{
    return m_waitingListSize;
}

auto CoreClient::getWaitingList() -> std::vector<Order>
{
    std::lock_guard<std::mutex> lk(m_mutex_waitingList);
    return m_waitingList;
}

void CoreClient::cancelAllPendingOrders()
{
    for (auto order : getWaitingList()) {
        submitCancellation(order);
    }

    // wait to make sure cancellation went through
    while (getWaitingListSize() != 0) {
        std::this_thread::sleep_for(1s);
    }
}

auto CoreClient::getOpenPrice(const std::string& symbol) -> double
{
    if (!isConnected()) {
        return 0.0;
    }

    return m_fixInitiator->getOpenPrice(symbol);
}

auto CoreClient::getClosePrice(const std::string& symbol, bool buy, int size) -> double
{
    if (!isConnected()) {
        return 0.0;
    }

    auto global = (buy) ? getOrderBookWithDestination(symbol, OrderBook::Type::GLOBAL_ASK) : getOrderBookWithDestination(symbol, OrderBook::Type::GLOBAL_BID);
    auto local = (buy) ? getOrderBookWithDestination(symbol, OrderBook::Type::LOCAL_ASK) : getOrderBookWithDestination(symbol, OrderBook::Type::LOCAL_BID);
    int buySign = ((buy) ? 1 : -1);

    double closePrice = 0.0;
    std::vector<double> prices;
    std::vector<int> sizes;

    auto globalIt = global.begin();
    bool globalDone = false;
    auto localIt = local.begin();
    bool localDone = false;
    auto bestPriceIt = globalIt;

    if (size < 0) {
        size = -size;
    }

    while (size != 0) {
        if (globalIt == global.end()) { // reached the end of the global order book
            globalDone = true;
            globalIt = localIt;
        }
        if (localIt == local.end()) { // reached the end of the local order book
            localDone = true;
            localIt = globalIt;
        }
        if (globalDone && localDone) { // reached the end of both order books
            break;
        }

        // which one has a better price?
        // - if buying: global < local -> global
        //              local >= global -> local
        // - if selling: -global < -local -> global
        //               -local >= -global -> local
        if ((buySign * globalIt->getPrice()) < (buySign * localIt->getPrice())) { // global has a better price
            bestPriceIt = globalIt;

            if (localIt == globalIt) {
                ++localIt;
            }
            ++globalIt;
        } else { // local has a better price
            bestPriceIt = localIt;

            if (globalIt == localIt) {
                ++globalIt;
            }
            ++localIt;
        }

        // add best price to the vector of prices
        prices.push_back(bestPriceIt->getPrice());

        // add best size to the vector of sizes
        if (size > bestPriceIt->getSize()) {
            sizes.push_back(bestPriceIt->getSize());
            size -= bestPriceIt->getSize();
        } else {
            sizes.push_back(size);
            size = 0;
        }
    }

    if (!prices.empty()) {
        closePrice = std::inner_product(prices.begin(), prices.end(), sizes.begin(), 0.0) / std::accumulate(sizes.begin(), sizes.end(), 0.0);
    }

    return closePrice;
}

auto CoreClient::getClosePrice(const std::string& symbol) -> double
{
    if (!isConnected()) {
        return 0.0;
    }

    int shares = getPortfolioItem(symbol).getShares();

    if (shares > 0) {
        return getClosePrice(symbol, false, shares / 100);
    }

    if (shares < 0) {
        return getClosePrice(symbol, true, shares / 100);
    }

    return 0.0;
}

auto CoreClient::getLastPrice(const std::string& symbol) -> double
{
    if (!isConnected()) {
        return 0.0;
    }

    return m_fixInitiator->getLastPrice(symbol);
}

auto CoreClient::getLastSize(const std::string& symbol) -> int
{
    if (!isConnected()) {
        return 0;
    }

    return m_fixInitiator->getLastSize(symbol);
}

auto CoreClient::getLastTradeTime() -> std::chrono::system_clock::time_point
{
    if (!isConnected()) {
        return std::chrono::system_clock::time_point();
    }

    return m_fixInitiator->getLastTradeTime();
}

auto CoreClient::getBestPrice(const std::string& symbol) -> BestPrice
{
    if (!isConnected()) {
        return BestPrice();
    }

    return m_fixInitiator->getBestPrice(symbol);
}

auto CoreClient::getOrderBook(const std::string& symbol, const OrderBook::Type& type, int maxLevel) -> std::vector<OrderBookEntry>
{
    if (!isConnected()) {
        return std::vector<OrderBookEntry>();
    }

    return m_fixInitiator->getOrderBook(symbol, type, maxLevel);
}

auto CoreClient::getOrderBookWithDestination(const std::string& symbol, const OrderBook::Type& type) -> std::vector<OrderBookEntry>
{
    if (!isConnected()) {
        return std::vector<OrderBookEntry>();
    }

    return m_fixInitiator->getOrderBookWithDestination(symbol, type);
}

auto CoreClient::getStockList() -> std::vector<std::string>
{
    if (!isConnected()) {
        return std::vector<std::string>();
    }

    return m_fixInitiator->getStockList();
}

void CoreClient::requestCompanyNames()
{
    if (!isConnected()) {
        return;
    }

    return m_fixInitiator->requestCompanyNames();
}

auto CoreClient::getCompanyNames() -> std::map<std::string, std::string>
{
    if (!isConnected()) {
        return std::map<std::string, std::string>();
    }

    return m_fixInitiator->getCompanyNames();
}

auto CoreClient::getCompanyName(const std::string& symbol) -> std::string
{
    if (!isConnected()) {
        return std::string();
    }

    return m_fixInitiator->getCompanyName(symbol);
}

auto CoreClient::requestSamplePrices(std::vector<std::string> symbols, double samplingFrequency /* = 1 */, unsigned int samplingWindow /* = 31 */) -> bool
{
    if (!isConnected()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> samplePricesFlagsGuard(m_mutex_samplePricesFlags);

        auto it = symbols.begin();
        while (it != symbols.end()) {
            if (m_samplePricesFlags[*it]) {
                it = symbols.erase(it);
            } else {
                m_samplePricesFlags[*it] = true;
                ++it;
            }
        }
    }

    if (symbols.begin() == symbols.end()) {
        return false;
    }

    m_samplePriceThreads.emplace_back(&CoreClient::calculateSamplePrices, this, symbols, samplingFrequency, samplingWindow);
    return true;
}

auto CoreClient::cancelSamplePricesRequest(const std::vector<std::string>& symbols) -> bool
{
    bool success = false;

    {
        std::lock_guard<std::mutex> samplePricesFlagsGuard(m_mutex_samplePricesFlags);

        for (const auto& symbol : symbols) {
            if (m_samplePricesFlags[symbol]) {
                m_samplePricesFlags[symbol] = false;
                success = true;
            }
        }
    }

    return success;
}

auto CoreClient::cancelAllSamplePricesRequests() -> bool
{
    bool success = false;

    {
        std::lock_guard<std::mutex> samplePricesFlagsGuard(m_mutex_samplePricesFlags);

        for (auto& kv : m_samplePricesFlags) {
            if (kv.second) {
                kv.second = false;
                success = true;
            }
        }
    }

    for (auto& t : m_samplePriceThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return success;
}

auto CoreClient::getSamplePricesSize(const std::string& symbol) -> int
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
    return static_cast<int>(m_sampleLastPrices[symbol].size());
}

auto CoreClient::getSamplePrices(const std::string& symbol, bool midPrices /* = false */) -> std::list<double>
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    if (midPrices) {
        return m_sampleMidPrices[symbol];
    }

    return m_sampleLastPrices[symbol];
}

auto CoreClient::getLogReturnsSize(const std::string& symbol) -> int
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
    return static_cast<int>(m_sampleLastPrices[symbol].empty() ? 0 : (m_sampleLastPrices[symbol].size() - 1));
}

auto CoreClient::getLogReturns(const std::string& symbol, bool midPrices /* = false */) -> std::list<double>
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    if (m_sampleLastPrices[symbol].size() < 2) {
        return std::list<double>();
    }

    // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
    std::list<double> logReturns(m_sampleLastPrices[symbol].size());

    if (midPrices) {
        std::transform(m_sampleMidPrices[symbol].begin(), m_sampleMidPrices[symbol].end(), logReturns.begin(), [](double x) { return std::log(x); });
    } else {
        std::transform(m_sampleLastPrices[symbol].begin(), m_sampleLastPrices[symbol].end(), logReturns.begin(), [](double x) { return std::log(x); });
    }

    std::adjacent_difference(logReturns.begin(), logReturns.end(), logReturns.begin());
    logReturns.pop_front();

    return logReturns;
}

auto CoreClient::subOrderBook(const std::string& symbol) -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->subOrderBook(symbol);

    return true;
}

auto CoreClient::unsubOrderBook(const std::string& symbol) -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->unsubOrderBook(symbol);

    return true;
}

auto CoreClient::subAllOrderBook() -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->subAllOrderBook();

    return true;
}

auto CoreClient::unsubAllOrderBook() -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->unsubAllOrderBook();

    return true;
}

auto CoreClient::getSubscribedOrderBookList() -> std::vector<std::string>
{
    if (!isConnected()) {
        return std::vector<std::string>();
    }

    return m_fixInitiator->getSubscribedOrderBookList();
}

auto CoreClient::subCandleData(const std::string& symbol) -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->subCandleData(symbol);

    return true;
}

auto CoreClient::unsubCandleData(const std::string& symbol) -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->unsubCandleData(symbol);

    return true;
}

auto CoreClient::subAllCandleData() -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->subAllCandleData();

    return true;
}

auto CoreClient::unsubAllCandleData() -> bool
{
    if (!isConnected()) {
        return false;
    }

    m_fixInitiator->unsubAllCandleData();

    return true;
}

auto CoreClient::getSubscribedCandlestickList() -> std::vector<std::string>
{
    if (!isConnected()) {
        return std::vector<std::string>();
    }

    return m_fixInitiator->getSubscribedCandlestickList();
}

/**
 * @brief Method to print debug messages.
 * @param message as a string contains content to be print.
 */
inline void CoreClient::debugDump(const std::string& message) const
{
    if (m_verbose) {
        cout << "CoreClient:" << endl;
        cout << message << endl;
    }
}

auto CoreClient::attachInitiator(FIXInitiator& initiator) -> bool
{
    // will attach one way
    m_fixInitiator = &initiator;
    return (m_fixInitiator != nullptr);
}

void CoreClient::storeExecution(const std::string& orderID, Order::Type orderType, int executedSize, double executedPrice, Order::Status newStatus)
{
    std::lock_guard<std::mutex> lk(m_mutex_orders);

    auto& order = m_submittedOrders[orderID];

    int newExecutedSize = order.getExecutedSize() + executedSize;
    double newExecutedPrice = order.getExecutedPrice();

    if (executedSize > 0) {
        if (order.getExecutedSize() > 0) {
            newExecutedPrice = ((order.getExecutedPrice() * order.getExecutedSize()) + (executedPrice * executedSize)) / newExecutedSize;
        } else {
            newExecutedPrice = executedPrice;
        }
    }

    order.setExecutedSize(newExecutedSize);
    order.setExecutedPrice(newExecutedPrice);

    // this is required in case cancellation orders are sent
    // before FILLED status is received, possibly originating:
    // - PENDING_CANCEL status from the Matching Engine
    // - REJECTED status from the Brokerage Center
    if (order.getStatus() != Order::Status::FILLED) {
        order.setStatus(newStatus);
    }

    if (newStatus == Order::Status::CANCELED) {
        // not everything was canceled -> there is still volume to be executed
        if (order.getSize() > newExecutedSize) {
            order.setStatus(Order::Status::PARTIALLY_FILLED);
        }
        // there is nothing else to be executed, but some was executed before the cancellation
        else if (newExecutedPrice > 0.0) {
            order.setStatus(Order::Status::FILLED);
        }
    }

    if ((newStatus == Order::Status::PARTIALLY_FILLED)
        || (newStatus == Order::Status::FILLED)
        || (newStatus == Order::Status::CANCELED)) {
        auto executedOrder = order;
        executedOrder.setType(orderType);
        executedOrder.setExecutedSize(executedSize);
        executedOrder.setExecutedPrice(executedPrice);
        m_executedOrders.insert({ orderID, executedOrder });
    }
}

void CoreClient::storePortfolioSummary(double totalBP, int totalShares, double totalRealizedPL)
{
    std::lock_guard<std::mutex> lk(m_mutex_portfolioSummary);

    if (!m_portfolioSummary.isOpenBPReady()) {
        m_portfolioSummary.setOpenBP(totalBP);
    }

    m_portfolioSummary.setTotalBP(totalBP);
    m_portfolioSummary.setTotalShares(totalShares);
    m_portfolioSummary.setTotalRealizedPL(totalRealizedPL);
    m_portfolioSummary.setTimestamp(); // current time
}

void CoreClient::storePortfolioItem(const std::string& symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL)
{
    std::lock_guard<std::mutex> lock(m_mutex_symbol_portfolioItem);
    m_symbol_portfolioItem[symbol] = PortfolioItem { symbol, longShares, shortShares, longPrice, shortPrice, realizedPL };
}

void CoreClient::storeWaitingList(std::vector<Order>&& waitingList)
{
    std::lock_guard<std::mutex> lock(m_mutex_waitingList);
    m_waitingList = std::move(waitingList);
    m_waitingListSize = m_waitingList.size();
}

void CoreClient::calculateSamplePrices(std::vector<std::string> symbols, double samplingFrequency, unsigned int samplingWindow)
{
    size_t ready = 0;
    BestPrice bp {};
    std::unordered_map<std::string, double> lastPrice;
    std::unordered_map<std::string, double> bestBid;
    std::unordered_map<std::string, double> bestAsk;

    // wait until first prices are available
    while (ready != symbols.size()) {
        ready = 0;
        for (const std::string& sym : symbols) {
            lastPrice[sym] = getLastPrice(sym);
            if (lastPrice[sym] > 0.0) {
                ++ready;
            }
        }
        std::this_thread::sleep_for(samplingFrequency * 1s);
    }

    while (symbols.begin() != symbols.end()) {
        for (const std::string& sym : symbols) {
            // last price
            lastPrice[sym] = getLastPrice(sym);

            // mid price
            bp = getBestPrice(sym);
            bestBid[sym] = (bp.getBidPrice() > 0.0) ? bp.getBidPrice() : lastPrice[sym];
            bestAsk[sym] = (bp.getAskPrice() > 0.0) ? bp.getAskPrice() : lastPrice[sym];

            {
                std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

                m_sampleLastPrices[sym].push_back(lastPrice[sym]);
                m_sampleMidPrices[sym].push_back((bestBid[sym] + bestAsk[sym]) / 2.0);

                // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
                if (m_sampleLastPrices[sym].size() > samplingWindow) {
                    m_sampleLastPrices[sym].pop_front();
                    m_sampleMidPrices[sym].pop_front();
                }
            }
        }

        std::this_thread::sleep_for(samplingFrequency * 1s);
        {
            std::lock_guard<std::mutex> samplePricesFlagsGuard(m_mutex_samplePricesFlags);
            std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

            auto it = symbols.begin();
            while (it != symbols.end()) {
                if (!m_samplePricesFlags[*it]) {
                    m_sampleLastPrices[*it].clear();
                    m_sampleMidPrices[*it].clear();
                    it = symbols.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

} // shift
