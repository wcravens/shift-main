#include "BCDocuments.h"
#include "DBConnector.h"

#include <shift/miscutils/Common.h>
#include <shift/miscutils/database/Common.h>

#include <algorithm>
#include <cassert>

using namespace std::chrono_literals;

//-------------------------------------------------------------------------------------------

/* static */ std::atomic<bool> BCDocuments::s_isSecurityListReady{ false };

/*static*/ BCDocuments* BCDocuments::getInstance()
{
    static BCDocuments s_BCDocInst;
    return &s_BCDocInst;
}

void BCDocuments::addSymbol(const std::string& symbol)
{ // here no locking is needed only when satisfies precondition: addSymbol was run before others
    m_symbols.insert(symbol);
}

bool BCDocuments::hasSymbol(const std::string& symbol) const
{
    return m_symbols.find(symbol) != m_symbols.end();
}

const std::unordered_set<std::string>& BCDocuments::getSymbols() const
{
    return m_symbols;
}

void BCDocuments::attachOrderBookToSymbol(const std::string& symbol)
{
    auto& orderBookPtr = m_orderBookBySymbol[symbol]; // insert new
    orderBookPtr.reset(new OrderBook(symbol));
    orderBookPtr->spawn();
}

void BCDocuments::attachCandlestickDataToSymbol(const std::string& symbol)
{
    auto& candlePtr = m_candleBySymbol[symbol]; // insert new
    candlePtr.reset(new CandlestickData);
    candlePtr->spawn();
}

void BCDocuments::registerUserInDoc(const std::string& targetID, const std::string& userID)
{
    std::lock_guard<std::mutex> guard(m_mtxUserID2TargetID);
    m_userID2TargetID[userID] = targetID;
    registerTarget(targetID);
}

// removes all users affiliated to the target computer ID
void BCDocuments::unregisterTargetFromDoc(const std::string& targetID)
{
    std::lock_guard<std::mutex> guard(m_mtxUserID2TargetID);

    unregisterTarget(targetID);

    std::vector<std::string> userIDs;
    for (auto& kv : m_userID2TargetID)
        if (targetID == kv.second)
            userIDs.push_back(kv.first);

    for (const auto& userID : userIDs)
        m_userID2TargetID.erase(userID);
}

std::string BCDocuments::getTargetIDByUserID(const std::string& userID) const
{
    std::lock_guard<std::mutex> guard(m_mtxUserID2TargetID);

    auto pos = m_userID2TargetID.find(userID);
    if (m_userID2TargetID.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

void BCDocuments::unregisterTargetFromOrderBooks(const std::string& targetID)
{
    std::unique_lock<std::mutex> lock(m_mtxOrderBookSymbolsByTargetID);
    auto pos = m_orderBookSymbolsByTargetID.find(targetID);
    if (m_orderBookSymbolsByTargetID.end() == pos)
        return;

    std::vector<std::string> symbolsCopy(pos->second.begin(), pos->second.end());
    m_orderBookSymbolsByTargetID.erase(pos);
    lock.unlock();

    for (const auto& symbol : symbolsCopy) {
        m_orderBookBySymbol[symbol]->onUnsubscribeOrderBook(targetID);
    }
}

void BCDocuments::unregisterTargetFromCandles(const std::string& targetID)
{
    std::unique_lock<std::mutex> lock(m_mtxCandleSymbolsByTargetID);
    auto pos = m_candleSymbolsByTargetID.find(targetID);
    if (m_candleSymbolsByTargetID.end() == pos)
        return;

    std::vector<std::string> symbolsCopy(pos->second.begin(), pos->second.end());
    m_candleSymbolsByTargetID.erase(pos);
    lock.unlock();

    for (const auto& symbol : symbolsCopy) {
        m_candleBySymbol[symbol]->unregisterUserInCandlestickData(targetID);
    }
}

bool BCDocuments::manageSubscriptionInOrderBook(bool isSubscribe, const std::string& symbol, const std::string& targetID)
{
    assert(s_isSecurityListReady);

    auto pos = m_orderBookBySymbol.find(symbol);
    if (m_orderBookBySymbol.end() == pos)
        return false; // unknown RIC

    if (isSubscribe) {
        pos->second->onSubscribeOrderBook(targetID);

        std::lock_guard<std::mutex> guard(m_mtxOrderBookSymbolsByTargetID);
        m_orderBookSymbolsByTargetID[targetID].insert(symbol);
    } else {
        pos->second->onUnsubscribeOrderBook(targetID);
    }

    return true;
}

bool BCDocuments::manageSubscriptionInCandlestickData(bool isSubscribe, const std::string& symbol, const std::string& targetID)
{
    assert(s_isSecurityListReady);
    /*  I do write assert(...) here instead of explicitly writting busy waiting loop because that
        assert() assumes that the dependency logics are implied and preconditions are guarenteed by the
        implementation somewhere, whereas a waiting loop assumes nothing.
        Hence, the assert() here is reasonable because this function is invoked only after we connected to the user, at which security list must be ready.
    */

    auto pos = m_candleBySymbol.find(symbol);
    if (m_candleBySymbol.end() == pos)
        return false; // unknown RIC

    if (isSubscribe) {
        pos->second->registerUserInCandlestickData(targetID);

        std::lock_guard<std::mutex> guard(m_mtxCandleSymbolsByTargetID);
        m_candleSymbolsByTargetID[targetID].insert(symbol);
    } else {
        pos->second->unregisterUserInCandlestickData(targetID);
    }

    return true;
}

int BCDocuments::sendHistoryToUser(const std::string& userID)
{
    int res = 0;
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByUserID);

    auto pos = m_riskManagementByUserID.find(userID);
    if (m_riskManagementByUserID.end() == pos) { // newly joined user ? (TODO)
        pos = addRiskManagementToUserNoLock(userID);
        res = 1;
    }
    pos->second->sendPortfolioHistory();
    pos->second->sendWaitingList();

    return res;
}

double BCDocuments::getOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const
{
    double globalPrice = 0.0;
    double localPrice = 0.0;

    auto pos = m_orderBookBySymbol.find(symbol);
    if (m_orderBookBySymbol.end() == pos)
        return 0.0;

    if (isBuy) {
        globalPrice = pos->second->getGlobalBidOrderBookFirstPrice();
        localPrice = pos->second->getLocalBidOrderBookFirstPrice();

        return std::max(globalPrice, localPrice);
    } else { // Sell
        globalPrice = pos->second->getGlobalAskOrderBookFirstPrice();
        localPrice = pos->second->getLocalAskOrderBookFirstPrice();

        if ((globalPrice && globalPrice < localPrice) || (localPrice == 0.0))
            return globalPrice;
        return localPrice;
    }
}

void BCDocuments::onNewOBUpdateForOrderBook(const std::string& symbol, OrderBookEntry&& update)
{
    if (!s_isSecurityListReady)
        return;
    m_orderBookBySymbol[symbol]->enqueueOrderBookUpdate(std::move(update));
}

void BCDocuments::onNewTransacForCandlestickData(const std::string& symbol, const Transaction& transac)
{
    if (!s_isSecurityListReady)
        return;
    m_candleBySymbol[symbol]->enqueueTransaction(transac);
}

void BCDocuments::onNewOrderForUserRiskManagement(const std::string& userID, Order&& order)
{
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByUserID);
    addRiskManagementToUserNoLock(userID);
    m_riskManagementByUserID[userID]->enqueueOrder(std::move(order));
}

void BCDocuments::onNewExecutionReportForUserRiskManagement(const std::string& userID, ExecutionReport&& report)
{
    if ("TRTH" == userID)
        return;

    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByUserID);
    m_riskManagementByUserID[userID]->enqueueExecRpt(std::move(report));
}

void BCDocuments::broadcastOrderBooks() const
{
    for (const auto& kv : m_orderBookBySymbol) {
        auto& orderBook = *kv.second;
        orderBook.broadcastWholeOrderBookToAll();
    }
}

std::unordered_map<std::string, std::unique_ptr<RiskManagement>>::iterator BCDocuments::addRiskManagementToUserNoLock(const std::string& userID)
{
    auto res = m_riskManagementByUserID.emplace(userID, nullptr);
    if (!res.second)
        return res.first;

    auto& rmPtr = res.first->second;

    auto lock{ DBConnector::getInstance()->lockPSQL() };

    const auto summary = shift::database::readFieldsOfRow(DBConnector::getInstance()->getConn(),
        "SELECT buying_power, holding_balance, borrowed_balance, total_pl, total_shares\n"
        "FROM portfolio_summary\n" // summary table MAYBE have got the user's id
        "WHERE id = '" // here presume we always has got current userID in traders table
            + userID + "';",
        5);

    if (summary.empty()) { // no expected user's uuid found in the summary table, therefore use a default summary?
        DBConnector::getInstance()->doQuery("INSERT INTO portfolio_summary (id, buying_power) VALUES ('" + userID + "'," + std::to_string(DEFAULT_BUYING_POWER) + ");", "");
        rmPtr.reset(new RiskManagement(userID, ::DEFAULT_BUYING_POWER));
    } else // explicitly parameterize the summary
        rmPtr.reset(new RiskManagement(userID, std::stod(summary[0]), std::stod(summary[1]), std::stod(summary[2]), std::stod(summary[3]), std::stoi(summary[4])));

    // populate portfolio items
    for (int row = 0; true; row++) {
        const auto& item = shift::database::readFieldsOfRow(DBConnector::getInstance()->getConn(),
            "SELECT symbol, borrowed_balance, pl, long_price, short_price, long_shares, short_shares\n"
            "FROM portfolio_items\n"
            "WHERE id = '"
                + userID + "';",
            7,
            row);
        if (item.empty())
            break;

        rmPtr->insertPortfolioItem(item[0], { item[0], std::stod(item[1]), std::stod(item[2]), std::stod(item[3]), std::stod(item[4]), std::stoi(item[5]), std::stoi(item[6]) });
    }

    rmPtr->spawn();

    return res.first;
}
