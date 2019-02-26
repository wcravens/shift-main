#include "BCDocuments.h"
#include "DBConnector.h"

#include <shift/miscutils/database/Common.h>

#include <algorithm>
#include <cassert>

using namespace std::chrono_literals;

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

const std::unordered_set<std::string>& BCDocuments::getSymbols() const
{
    return m_symbols;
}

bool BCDocuments::hasSymbol(const std::string& symbol) const
{
    return m_symbols.find(symbol) != m_symbols.end();
}

void BCDocuments::attachStockToSymbol(const std::string& symbol)
{
    auto& stockPtr = m_stockBySymbol[symbol]; // insert new
    stockPtr.reset(new Stock(symbol));
    stockPtr->spawn();
}

void BCDocuments::addOrderBookEntryToStock(const std::string& symbol, const OrderBookEntry& entry)
{
    while (!s_isSecurityListReady)
        std::this_thread::sleep_for(500ms);
    m_stockBySymbol[symbol]->enqueueOrderBook(entry);
}

void BCDocuments::attachCandlestickDataToSymbol(const std::string& symbol)
{
    auto& candlePtr = m_candleBySymbol[symbol]; // insert new
    candlePtr.reset(new CandlestickData);
    candlePtr->spawn();
}

void BCDocuments::addTransacToCandlestickData(const std::string& symbol, const Transaction& transac)
{
    while (!s_isSecurityListReady)
        std::this_thread::sleep_for(500ms);
    m_candleBySymbol[symbol]->enqueueTransaction(transac);
}

void BCDocuments::addRiskManagementToUserLockedExplicit(const std::string& userName, double buyingPower, int shares, double price, const std::string& symbol)
{ // for low frequency client use
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);

    auto res = m_riskManagementByName.emplace(userName, nullptr);
    if (res.second) {
        auto& rmPtr = res.first->second;
        rmPtr.reset(new RiskManagement(userName, buyingPower, shares));
        rmPtr->insertPortfolioItem(symbol, { symbol, shares, price });
        rmPtr->spawn();
    }
}

auto BCDocuments::addRiskManagementToUserNoLock(const std::string& userName) -> decltype(m_riskManagementByName)::iterator
{
    auto res = m_riskManagementByName.emplace(userName, nullptr);
    if (!res.second)
        return res.first;

    auto& rmPtr = res.first->second;

    auto lock{ DBConnector::getInstance()->lockPSQL() };

    const auto summary = shift::database::readFieldsOfRow(DBConnector::getInstance()->getConn(),
        "SELECT buying_power, holding_balance, borrowed_balance, total_pl, total_shares\n"
        "FROM traders INNER JOIN portfolio_summary ON traders.id = portfolio_summary.id\n" // summary table MAYBE have got the user's id
        "WHERE traders.username = '" // here presume we always has got current user name in traders table
            + userName + "';",
        5);

    if (summary.empty()) { // no expected user's uuid found in the summary table, therefore use a default summary?
        constexpr auto DEFAULT_BUYING_POWER = 1e6;
        DBConnector::getInstance()->doQuery("INSERT INTO portfolio_summary (id, buying_power) VALUES ((SELECT id FROM traders WHERE username = '" + userName + "'), " + std::to_string(DEFAULT_BUYING_POWER) + ");", "");
        rmPtr.reset(new RiskManagement(userName, DEFAULT_BUYING_POWER));
    } else // explicitly parameterize the summary
        rmPtr.reset(new RiskManagement(userName, std::stod(summary[0]), std::stod(summary[1]), std::stod(summary[2]), std::stod(summary[3]), std::stoi(summary[4])));

    // populate portfolio items
    for (int row = 0; true; row++) {
        const auto& item = shift::database::readFieldsOfRow(DBConnector::getInstance()->getConn(),
            "SELECT symbol, borrowed_balance, pl, long_price, short_price, long_shares, short_shares\n"
            "FROM traders INNER JOIN portfolio_items ON traders.id = portfolio_items.id\n"
            "WHERE traders.username = '"
                + userName + "';",
            7,
            row);
        if (item.empty())
            break;

        rmPtr->insertPortfolioItem(item[0], { item[0], std::stod(item[1]), std::stod(item[2]), std::stod(item[3]), std::stod(item[4]), std::stoi(item[5]), std::stoi(item[6]) });
    }

    rmPtr->spawn();

    return res.first;
}

void BCDocuments::addQuoteToUserRiskManagement(const std::string& userName, const Quote& quote)
{
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);
    addRiskManagementToUserNoLock(userName);
    m_riskManagementByName[userName]->enqueueQuote(quote);
}

void BCDocuments::addReportToUserRiskManagement(const std::string& userName, const Report& report)
{
    if ("TR" == userName)
        return;
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);
    m_riskManagementByName[userName]->enqueueExecRpt(report);
}

double BCDocuments::getStockOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const
{
    double global_price = 0.0;
    double local_price = 0.0;

    auto pos = m_stockBySymbol.find(symbol);
    if (m_stockBySymbol.end() == pos)
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

bool BCDocuments::manageUsersInStockOrderBook(bool isRegister, const std::string& symbol, const std::string& userName) const
{
    auto pos = m_stockBySymbol.find(symbol);
    if (m_stockBySymbol.end() != pos) {
        if (isRegister)
            pos->second->registerUserInStock(userName);
        else // unregister
            pos->second->unregisterUserInStock(userName);
        return true;
    }

    return false;
}

bool BCDocuments::manageUsersInCandlestickData(bool isRegister, const std::string& userName, const std::string& symbol)
{
    assert(s_isSecurityListReady);
    /*  ||
       \||/
        \/
        I do write assert(...) here instead of explicitly writting busy waiting loop because that
        assert() assumes that the dependency logics are implied and preconditions are guarenteed by the
        implementation somewhere, whereas a waiting loop assumes nothing.
        Hence, the assert() here is reasonable because this function is invoked only after we connected to the user, at which security list must be ready.
    */

    auto pos = m_candleBySymbol.find(symbol);
    if (m_candleBySymbol.end() != pos) {
        if (isRegister) {
            pos->second->registerUserInCandlestickData(userName);

            std::lock_guard<std::mutex> guard(m_mtxCandleSymbolsByName);
            m_candleSymbolsByName[userName].insert(symbol);
        } else
            pos->second->unregisterUserInCandlestickData(userName);
        return true;
    }

    return false;
}

int BCDocuments::sendHistoryToUser(const std::string& userName)
{
    int res = 0;
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);

    auto pos = m_riskManagementByName.find(userName);
    if (m_riskManagementByName.end() == pos) { // newly joined user ?
        pos = addRiskManagementToUserNoLock(userName);
        res = 1;
    }
    pos->second->sendPortfolioHistory();
    pos->second->sendQuoteHistory();

    return res;
}

std::unordered_map<std::string, std::string> BCDocuments::getConnectedTargetIDsMap()
{
    std::lock_guard<std::mutex> guard(m_mtxMapsBetweenNamesAndTargetIDs);
    return m_mapTarID2Name;
}

void BCDocuments::registerUserInDoc(const std::string& userName, const std::string& targetID)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxMapsBetweenNamesAndTargetIDs);
    m_mapTarID2Name[targetID] = userName;
    m_mapName2TarID[userName] = targetID;
}

void BCDocuments::unregisterUserInDoc(const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxMapsBetweenNamesAndTargetIDs);

    auto posN2ID = m_mapName2TarID.find(userName);
    if (m_mapName2TarID.end() != posN2ID) {
        const auto& tarID = posN2ID->second;
        m_mapTarID2Name.erase(tarID);
        m_mapName2TarID.erase(posN2ID);
    }
}

void BCDocuments::registerWebUserInDoc(const std::string& userName, const std::string& targetID)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxMapsBetweenNamesAndTargetIDs);
    m_mapName2TarID[userName] = targetID;
}

std::string BCDocuments::getTargetIDByUserName(const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxMapsBetweenNamesAndTargetIDs);

    auto pos = m_mapName2TarID.find(userName);
    if (m_mapName2TarID.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

std::string BCDocuments::getUserNameByTargetID(const std::string& targetID)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxMapsBetweenNamesAndTargetIDs);

    auto pos = m_mapTarID2Name.find(targetID);
    if (m_mapTarID2Name.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

void BCDocuments::addOrderBookSymbolToUser(const std::string& userName, const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxOrderBookSymbolsByName);
    m_orderBookSymbolsByName[userName].insert(symbol);
}

void BCDocuments::removeUserFromStocks(const std::string& userName)
{
    std::lock_guard<std::mutex> guardOSBN(m_mtxOrderBookSymbolsByName);

    auto pos = m_orderBookSymbolsByName.find(userName);
    if (m_orderBookSymbolsByName.end() != pos) {
        for (const auto& symbol : pos->second) {
            m_stockBySymbol[symbol]->unregisterUserInStock(userName);
        }
        m_orderBookSymbolsByName.erase(pos);
    }
}

void BCDocuments::removeUserFromCandles(const std::string& userName)
{
    std::lock_guard<std::mutex> guardCSBN(m_mtxCandleSymbolsByName);

    auto pos = m_candleSymbolsByName.find(userName);
    if (m_candleSymbolsByName.end() != pos) {
        for (const auto& symbol : pos->second) {
            m_candleBySymbol[symbol]->unregisterUserInCandlestickData(userName);
        }
        m_candleSymbolsByName.erase(pos);
    }
}

void BCDocuments::removeCandleSymbolFromUser(const std::string& userName, const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxCandleSymbolsByName);

    auto pos = m_candleSymbolsByName.find(userName);
    if (m_candleSymbolsByName.end() != pos) {
        pos->second.erase(symbol);
    }
}

void BCDocuments::broadcastStocks() const
{
    for (const auto& kv : m_stockBySymbol) {
        auto& stock = *kv.second;
        stock.broadcastWholeOrderBookToAll();
    }
}

//-------------------------------------------------------------------------------------------

std::string toUpper(const std::string& str)
{
    std::string upStr;
    std::transform(str.begin(), str.end(), std::back_inserter(upStr), [](char ch) { return std::toupper(ch); });
    return upStr;
}
