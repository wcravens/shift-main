#include "BCDocuments.h"
#include "DBConnector.h"

#include <shift/miscutils/database/Common.h>

#include <algorithm>
#include <cassert>

using namespace std::chrono_literals;

std::string toUpper(const std::string& str)
{
    std::string upStr;
    std::transform(str.begin(), str.end(), std::back_inserter(upStr), [](char ch) { return std::toupper(ch); });
    return upStr;
}

std::string toLower(const std::string& str)
{
    std::string upStr;
    std::transform(str.begin(), str.end(), std::back_inserter(upStr), [](char ch) { return std::tolower(ch); });
    return upStr;
}

//-------------------------------------------------------------------------------------------

/* static */ std::atomic<bool> BCDocuments::s_isSecurityListReady{ false };

/*static*/ BCDocuments* BCDocuments::getInstance()
{
    static BCDocuments s_BCDocInst;
    return &s_BCDocInst;
}

void BCDocuments::addSymbol(const std::string& symbol)
{ // Here no locking is needed only when satisfies precondition: addSymbol was run before others
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

void BCDocuments::onNewOBEntryForOrderBook(const std::string& symbol, const OrderBookEntry& entry)
{
    while (!s_isSecurityListReady)
        std::this_thread::sleep_for(500ms);
    m_orderBookBySymbol[symbol]->enqueueOrderBook(entry);
}

void BCDocuments::attachCandlestickDataToSymbol(const std::string& symbol)
{
    auto& candlePtr = m_candleBySymbol[symbol]; // insert new
    candlePtr.reset(new CandlestickData);
    candlePtr->spawn();
}

void BCDocuments::onNewTransacForCandlestickData(const std::string& symbol, const Transaction& transac)
{
    while (!s_isSecurityListReady)
        std::this_thread::sleep_for(500ms);
    m_candleBySymbol[symbol]->enqueueTransaction(transac);
}

auto BCDocuments::addRiskManagementToUserNoLock(const std::string& username) -> decltype(m_riskManagementByName)::iterator
{
    auto res = m_riskManagementByName.emplace(username, nullptr);
    if (!res.second)
        return res.first;

    auto& rmPtr = res.first->second;

    auto lock{ DBConnector::getInstance()->lockPSQL() };

    const auto summary = shift::database::readFieldsOfRow(DBConnector::getInstance()->getConn(),
        "SELECT buying_power, holding_balance, borrowed_balance, total_pl, total_shares\n"
        "FROM traders INNER JOIN portfolio_summary ON traders.id = portfolio_summary.id\n" // summary table MAYBE have got the user's id
        "WHERE traders.username = '" // here presume we always has got current username in traders table
            + username + "';",
        5);

    if (summary.empty()) { // no expected user's uuid found in the summary table, therefore use a default summary?
        constexpr auto DEFAULT_BUYING_POWER = 1e6;
        DBConnector::getInstance()->doQuery("INSERT INTO portfolio_summary (id, buying_power) VALUES ((SELECT id FROM traders WHERE username = '" + username + "'), " + std::to_string(DEFAULT_BUYING_POWER) + ");", "");
        rmPtr.reset(new RiskManagement(username, DEFAULT_BUYING_POWER));
    } else // explicitly parameterize the summary
        rmPtr.reset(new RiskManagement(username, std::stod(summary[0]), std::stod(summary[1]), std::stod(summary[2]), std::stod(summary[3]), std::stoi(summary[4])));

    // populate portfolio items
    for (int row = 0; true; row++) {
        const auto& item = shift::database::readFieldsOfRow(DBConnector::getInstance()->getConn(),
            "SELECT symbol, borrowed_balance, pl, long_price, short_price, long_shares, short_shares\n"
            "FROM traders INNER JOIN portfolio_items ON traders.id = portfolio_items.id\n"
            "WHERE traders.username = '"
                + username + "';",
            7,
            row);
        if (item.empty())
            break;

        rmPtr->insertPortfolioItem(item[0], { item[0], std::stod(item[1]), std::stod(item[2]), std::stod(item[3]), std::stod(item[4]), std::stoi(item[5]), std::stoi(item[6]) });
    }

    rmPtr->spawn();

    return res.first;
}

void BCDocuments::onNewOrderForUserRiskManagement(const std::string& username, Order&& order)
{
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);
    addRiskManagementToUserNoLock(username);
    m_riskManagementByName[username]->enqueueOrder(std::move(order));
}

void BCDocuments::onNewReportForUserRiskManagement(const std::string& username, const Report& report)
{
    if ("TR" == username)
        return;
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);
    m_riskManagementByName[username]->enqueueExecRpt(report);
}

double BCDocuments::getOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const
{
    double global_price = 0.0;
    double local_price = 0.0;

    auto pos = m_orderBookBySymbol.find(symbol);
    if (m_orderBookBySymbol.end() == pos)
        return 0.0;

    if (isBuy) {
        global_price = pos->second->getGlobalBidOrderBookFirstPrice();
        local_price = pos->second->getLocalBidOrderBookFirstPrice();

        return std::max(global_price, local_price);
    } else { // Sell
        global_price = pos->second->getGlobalAskOrderBookFirstPrice();
        local_price = pos->second->getLocalAskOrderBookFirstPrice();

        if ((global_price && global_price < local_price) || (local_price == 0.0))
            return global_price;
        return local_price;
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

int BCDocuments::sendHistoryToUser(const std::string& username)
{
    int res = 0;
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);

    auto pos = m_riskManagementByName.find(username);
    if (m_riskManagementByName.end() == pos) { // newly joined user ? (TODO)
        pos = addRiskManagementToUserNoLock(username);
        res = 1;
    }
    pos->second->sendPortfolioHistory();
    pos->second->sendWaitingList();

    return res;
}

void BCDocuments::registerUserInDoc(const std::string& targetID, const std::string& username)
{
    std::lock_guard<std::mutex> guard(m_mtxUserTargetInfo);
    m_mapName2TarID[username] = targetID;
    registerTarget(targetID);
}

// removes all users affiliated to the target computer ID
void BCDocuments::unregisterTargetFromDoc(const std::string& targetID)
{
    std::lock_guard<std::mutex> guard(m_mtxUserTargetInfo);

    unregisterTarget(targetID);

    std::vector<std::string> usernames;
    for (auto& kv : m_mapName2TarID)
        if (targetID == kv.second)
            usernames.push_back(kv.first);

    for (auto& name : usernames)
        m_mapName2TarID.erase(name);
}

std::string BCDocuments::getTargetIDByUsername(const std::string& username) const
{
    std::lock_guard<std::mutex> guard(m_mtxUserTargetInfo);

    auto pos = m_mapName2TarID.find(username);
    if (m_mapName2TarID.end() == pos)
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

void BCDocuments::broadcastOrderBooks() const
{
    for (const auto& kv : m_orderBookBySymbol) {
        auto& orderBook = *kv.second;
        orderBook.broadcastWholeOrderBookToAll();
    }
}
