#include "CoreClient.h"

#include "FIXInitiator.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <thread>

#ifdef _WIN32
#include <terminal/Common.h>
#else
#include <shift/miscutils/terminal/Common.h>
#endif

using namespace std::chrono_literals;

shift::CoreClient::CoreClient()
    : m_fixInitiator{ nullptr }
    , m_verbose{ false }
    , m_submittedOrdersSize{ 0 }
    , m_waitingListSize{ 0 }
{
}

shift::CoreClient::CoreClient(const std::string& username)
    : m_fixInitiator{ nullptr }
    , m_username{ username }
    , m_verbose{ false }
    , m_submittedOrdersSize{ 0 }
    , m_waitingListSize{ 0 }
{
}

/**
 * @brief Default destructor for CoreClient object.
 */
shift::CoreClient::~CoreClient(void)
{
}

void shift::CoreClient::setVerbose(bool verbose)
{
    m_verbose = verbose;
}

bool shift::CoreClient::isVerbose()
{
    return m_verbose;
}

bool shift::CoreClient::isConnected()
{
    return (m_fixInitiator && m_fixInitiator->isConnected());
}

/** 
 * @brief Method to set username and password for current session.
 * @param username The username to be set as type string.
 * @param password The password to be set as type string.
 */
void shift::CoreClient::setUsername(const std::string& username)
{
    m_username = username;
}

std::string shift::CoreClient::getUsername()
{
    return m_username;
}

std::vector<shift::CoreClient*> shift::CoreClient::getAttachedClients()
{
    if (!isConnected()) {
        return std::vector<shift::CoreClient*>();
    }

    return m_fixInitiator->getAttachedClients();
}

/**
 * @brief Method to submit order from Core Client to Brokerage Center.
 * @param order as a shift::Order object contains all required information.
 */
void shift::CoreClient::submitOrder(const shift::Order& order)
{
    if (!isConnected()) {
        return;
    }

    if (order.getType() != shift::Order::CANCEL_BID && order.getType() != shift::Order::CANCEL_ASK) {
        std::lock_guard<std::mutex> lk(m_mutex_submittedOrders);
        m_submittedOrdersIDs.push_back(order.getID());
        m_submittedOrders[order.getID()] = order;
        m_submittedOrdersSize++;
    }

    return m_fixInitiator->submitOrder(order, m_username);
}

shift::PortfolioSummary shift::CoreClient::getPortfolioSummary()
{
    std::lock_guard<std::mutex> lk(m_mutex_portfolioSummary);
    return m_portfolioSummary;
}

std::map<std::string, shift::PortfolioItem> shift::CoreClient::getPortfolioItems()
{
    std::lock_guard<std::mutex> lock(m_mutex_symbol_portfolioItem);
    return m_symbol_portfolioItem;
}

shift::PortfolioItem shift::CoreClient::getPortfolioItem(const std::string& symbol)
{
    std::lock_guard<std::mutex> lock(m_mutex_symbol_portfolioItem);
    return m_symbol_portfolioItem[symbol];
}

int shift::CoreClient::getSubmittedOrdersSize()
{
    return m_submittedOrdersSize;
}

std::vector<shift::Order> shift::CoreClient::getSubmittedOrders()
{
    std::lock_guard<std::mutex> lk(m_mutex_submittedOrders);
    std::vector<shift::Order> submittedOrders;
    for (const std::string& id : m_submittedOrdersIDs) {
        submittedOrders.push_back(m_submittedOrders[id]);
    }
    return submittedOrders;
}

int shift::CoreClient::getWaitingListSize()
{
    return m_waitingListSize;
}

std::vector<shift::Order> shift::CoreClient::getWaitingList()
{
    std::lock_guard<std::mutex> lk(m_mutex_waitingList);
    return m_waitingList;
}

void shift::CoreClient::cancelAllPendingOrders()
{
    for (auto order : getWaitingList()) {
        if (order.getType() == shift::Order::LIMIT_BUY) {
            order.setType(shift::Order::CANCEL_BID);
            submitOrder(order);
        } else if (order.getType() == shift::Order::LIMIT_SELL) {
            order.setType(shift::Order::CANCEL_ASK);
            submitOrder(order);
        }
    }

    // Wait to make sure cancellation went through
    while (getWaitingListSize() != 0) {
        std::this_thread::sleep_for(1s);
    }
}

double shift::CoreClient::getOpenPrice(const std::string& symbol)
{
    if (!isConnected()) {
        return 0.0;
    }

    return m_fixInitiator->getOpenPrice(symbol);
}

double shift::CoreClient::getClosePrice(const std::string& symbol, bool buy, int size)
{
    if (!isConnected()) {
        return 0.0;
    }

    auto global = (buy) ? getOrderBookWithDestination(symbol, shift::OrderBook::Type::GLOBAL_ASK) : getOrderBookWithDestination(symbol, shift::OrderBook::Type::GLOBAL_BID);
    auto local = (buy) ? getOrderBookWithDestination(symbol, shift::OrderBook::Type::LOCAL_ASK) : getOrderBookWithDestination(symbol, shift::OrderBook::Type::LOCAL_BID);
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
        if (globalIt == global.end()) { // Reached the end of the global order book
            globalDone = true;
            globalIt = localIt;
        }
        if (localIt == local.end()) { // Reached the end of the local order book
            localDone = true;
            localIt = globalIt;
        }
        if (globalDone && localDone) { // Reached the end of both order books
            break;
        }

        // Which one has a better price?
        // - If buying: global < local -> global
        //              local >= global -> local
        // - If selling: -global < -local -> global
        //               -local >= -global -> local
        if ((buySign * globalIt->getPrice()) < (buySign * localIt->getPrice())) { // Global has a better price
            bestPriceIt = globalIt;

            if (localIt == globalIt) {
                ++localIt;
            }
            ++globalIt;
        } else { // Local has a better price
            bestPriceIt = localIt;

            if (globalIt == localIt) {
                ++globalIt;
            }
            ++localIt;
        }

        // Add best price to the vector of prices
        prices.push_back(bestPriceIt->getPrice());

        // Add best size to the vector of sizes
        if (size > bestPriceIt->getSize()) {
            sizes.push_back(bestPriceIt->getSize());
            size -= bestPriceIt->getSize();
        } else {
            sizes.push_back(size);
            size = 0;
        }
    }

    if (prices.size() > 0) {
        closePrice = std::inner_product(prices.begin(), prices.end(), sizes.begin(), 0.0) / std::accumulate(sizes.begin(), sizes.end(), 0.0);
    }

    return closePrice;
}

double shift::CoreClient::getLastPrice(const std::string& symbol)
{
    if (!isConnected()) {
        return 0.0;
    }

    return m_fixInitiator->getLastPrice(symbol);
}

int shift::CoreClient::getLastSize(const std::string& symbol)
{
    if (!isConnected()) {
        return 0;
    }

    return m_fixInitiator->getLastSize(symbol);
}

std::chrono::system_clock::time_point shift::CoreClient::getLastTradeTime()
{
    if (!isConnected()) {
        return std::chrono::system_clock::time_point();
    }

    return m_fixInitiator->getLastTradeTime();
}

shift::BestPrice shift::CoreClient::getBestPrice(const std::string& symbol)
{
    if (!isConnected()) {
        return shift::BestPrice();
    }

    return m_fixInitiator->getBestPrice(symbol);
}

std::vector<shift::OrderBookEntry> shift::CoreClient::getOrderBook(const std::string& symbol, const OrderBook::Type& type, int maxLevel)
{
    if (!isConnected()) {
        return std::vector<shift::OrderBookEntry>();
    }

    return m_fixInitiator->getOrderBook(symbol, type, maxLevel);
}

std::vector<shift::OrderBookEntry> shift::CoreClient::getOrderBookWithDestination(const std::string& symbol, const OrderBook::Type& type)
{
    if (!isConnected()) {
        return std::vector<shift::OrderBookEntry>();
    }

    return m_fixInitiator->getOrderBookWithDestination(symbol, type);
}

std::vector<std::string> shift::CoreClient::getStockList()
{
    if (!isConnected()) {
        return std::vector<std::string>();
    }

    return m_fixInitiator->getStockList();
}

void shift::CoreClient::requestCompanyNames()
{
    if (!isConnected()) {
        return;
    }

    return m_fixInitiator->requestCompanyNames();
}

std::map<std::string, std::string> shift::CoreClient::getCompanyNames()
{
    if (!isConnected()) {
        return std::map<std::string, std::string>();
    }

    return m_fixInitiator->getCompanyNames();
}

std::string shift::CoreClient::getCompanyName(const std::string& symbol)
{
    if (!isConnected()) {
        return std::string();
    }

    return m_fixInitiator->getCompanyName(symbol);
}

bool shift::CoreClient::requestSamplePrices(std::vector<std::string> symbols, double samplingFrequency, unsigned int samplingWindow)
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

    if (symbols.begin() != symbols.end()) {
        m_samplePriceThreads.push_back(std::thread(&shift::CoreClient::calculateSamplePrices, this, symbols, samplingFrequency, samplingWindow));
        return true;
    } else {
        return false;
    }
}

bool shift::CoreClient::cancelSamplePricesRequest(const std::vector<std::string>& symbols)
{
    bool success = false;

    {
        std::lock_guard<std::mutex> samplePricesFlagsGuard(m_mutex_samplePricesFlags);

        for (auto it = symbols.begin(); it != symbols.end(); ++it) {
            if (m_samplePricesFlags[*it]) {
                m_samplePricesFlags[*it] = false;
                success = true;
            }
        }
    }

    return success;
}

bool shift::CoreClient::cancelAllSamplePricesRequests()
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

int shift::CoreClient::getSamplePricesSize(const std::string& symbol)
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
    return static_cast<int>(m_sampleLastPrices[symbol].size());
}

std::list<double> shift::CoreClient::getSamplePrices(const std::string& symbol, bool midPrices)
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    if (!midPrices) {
        return m_sampleLastPrices[symbol];
    } else {
        return m_sampleMidPrices[symbol];
    }
}

int shift::CoreClient::getLogReturnsSize(const std::string& symbol)
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
    return static_cast<int>((m_sampleLastPrices[symbol].size() == 0) ? 0 : (m_sampleLastPrices[symbol].size() - 1));
}

std::list<double> shift::CoreClient::getLogReturns(const std::string& symbol, bool midPrices)
{
    std::lock_guard<std::mutex> samplePricesGuard(m_mutex_samplePrices);

    if (m_sampleLastPrices[symbol].size() > 1) {
        // m_sampleLastPrices[sym].size() == m_sampleMidPrices[sym].size()
        std::list<double> logReturns(m_sampleLastPrices[symbol].size());

        if (!midPrices) {
            std::transform(m_sampleLastPrices[symbol].begin(), m_sampleLastPrices[symbol].end(), logReturns.begin(), [](double x) { return std::log(x); });
        } else {
            std::transform(m_sampleMidPrices[symbol].begin(), m_sampleMidPrices[symbol].end(), logReturns.begin(), [](double x) { return std::log(x); });
        }

        std::adjacent_difference(logReturns.begin(), logReturns.end(), logReturns.begin());
        logReturns.pop_front();

        return logReturns;
    } else {
        return std::list<double>();
    }
}

bool shift::CoreClient::subOrderBook(const std::string& symbol)
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->subOrderBook(symbol);
}

bool shift::CoreClient::unsubOrderBook(const std::string& symbol)
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->unsubOrderBook(symbol);
}

bool shift::CoreClient::subAllOrderBook()
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->subAllOrderBook();
}

bool shift::CoreClient::unsubAllOrderBook()
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->unsubAllOrderBook();
}

std::vector<std::string> shift::CoreClient::getSubscribedOrderBookList()
{
    if (!isConnected()) {
        return std::vector<std::string>();
    }

    return m_fixInitiator->getSubscribedOrderBookList();
}

bool shift::CoreClient::subCandleData(const std::string& symbol)
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->subCandleData(symbol);
}

bool shift::CoreClient::unsubCandleData(const std::string& symbol)
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->unsubCandleData(symbol);
}

bool shift::CoreClient::subAllCandleData()
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->subAllCandleData();
}

bool shift::CoreClient::unsubAllCandleData()
{
    if (!isConnected()) {
        return false;
    }

    return m_fixInitiator->unsubAllCandleData();
}

std::vector<std::string> shift::CoreClient::getSubscribedCandlestickList()
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
inline void shift::CoreClient::debugDump(const std::string& message)
{
    if (m_verbose) {
        cout << "CoreClient:" << endl;
        cout << message << endl;
    }
}

bool shift::CoreClient::attach(FIXInitiator& initiator)
{
    // will attach one way
    m_fixInitiator = &initiator;
    return true;
}

void shift::CoreClient::storePortfolioItem(const std::string& symbol, int longShares, int shortShares, double longPrice, double shortPrice, double realizedPL)
{
    std::lock_guard<std::mutex> lock(m_mutex_symbol_portfolioItem);
    m_symbol_portfolioItem[symbol] = PortfolioItem{ symbol, longShares, shortShares, longPrice, shortPrice, realizedPL };
}

void shift::CoreClient::storePortfolioSummary(double totalBP, int totalShares, double totalRealizedPL)
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

void shift::CoreClient::storeWaitingList(std::vector<shift::Order> waitingList)
{
    std::lock_guard<std::mutex> lock(m_mutex_waitingList);
    m_waitingList = std::move(waitingList);
    m_waitingListSize = m_waitingList.size();
}

void shift::CoreClient::calculateSamplePrices(std::vector<std::string> symbols, double samplingFrequency, unsigned int samplingWindow)
{
    unsigned int ready = 0;
    double price = 0.0;
    std::map<std::string, double> lastPrice;
    std::map<std::string, double> bestBid;
    std::map<std::string, double> bestAsk;

    while (ready != symbols.size()) {
        ready = 0;
        for (const std::string& sym : symbols) {
            // Last price
            price = getLastPrice(sym);
            lastPrice[sym] = (price == 0.0) ? lastPrice[sym] : price;

            // Mid price
            price = getBestPrice(sym).getBidPrice();
            bestBid[sym] = (price == 0.0) ? bestBid[sym] : price;
            price = getBestPrice(sym).getAskPrice();
            bestAsk[sym] = (price == 0.0) ? bestAsk[sym] : price;

            if (lastPrice[sym] != 0.0 && bestBid[sym] != 0.0 && bestAsk[sym] != 0.0) { // Wait until first prices are available
                ready++;
            }
        }
        std::this_thread::sleep_for(samplingFrequency * 1s);
    }

    while (symbols.begin() != symbols.end()) {
        for (const std::string& sym : symbols) {
            // Last price
            price = getLastPrice(sym);
            lastPrice[sym] = (price == 0.0) ? lastPrice[sym] : price;

            // Mid price
            price = getBestPrice(sym).getBidPrice();
            bestBid[sym] = (price == 0.0) ? bestBid[sym] : price;
            price = getBestPrice(sym).getAskPrice();
            bestAsk[sym] = (price == 0.0) ? bestAsk[sym] : price;

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
