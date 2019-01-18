#include "BCDocuments.h"
#include "DBConnector.h"

#include <algorithm>

/*static*/ BCDocuments* BCDocuments::getInstance()
{
    static BCDocuments s_BCDocInst;
    return &s_BCDocInst;
}

void BCDocuments::addSymbol(const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxSymbols);
    m_symbols.insert(symbol);
}

std::unordered_set<std::string> BCDocuments::getSymbols()
{
    std::lock_guard<std::mutex> guard(m_mtxSymbols);
    return m_symbols;
}

void BCDocuments::addStockToSymbol(const std::string& symbol, const OrderBookEntry& ob)
{
    std::lock_guard<std::mutex> guard(m_mtxStockBySymbol);

    auto res = m_stockBySymbol.emplace(symbol, nullptr);
    auto& stockPtr = res.first->second;
    if (res.second) { // inserted new?
        stockPtr.reset(new Stock(symbol));
        stockPtr->spawn();
    }
    stockPtr->enqueueOrderBook(ob);
}

void BCDocuments::addCandleToSymbol(const std::string& symbol, const Transaction& transac)
{
    std::lock_guard<std::mutex> guard(m_mtxCandleBySymbol);

    auto res = m_candleBySymbol.emplace(symbol, nullptr);
    auto& candPtr = res.first->second;
    if (res.second) { // inserted new?
        candPtr.reset(
            new CandlestickData(
                transac.symbol,
                transac.price,
                transac.price,
                transac.price,
                transac.price,
                transac.price,
                transac.execTime.getTimeT()));
        candPtr->spawn();
    }
    candPtr->enqueueTransaction(transac);
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
    if (res.second) {
        auto& rmPtr = res.first->second;

        auto summary = DBConnector::s_readFieldsOfRow(
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
            const auto& item = DBConnector::s_readFieldsOfRow(
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
    }
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
    std::lock_guard<std::mutex> guard(m_mtxStockBySymbol);

    auto pos = m_stockBySymbol.find(symbol);
    if (m_stockBySymbol.end() == pos)
        return 0.0;

    if (isBuy) {
        global_price = pos->second->getGlobalBidOrderbookFirstPrice();
        local_price = pos->second->getLocalBidOrderbookFirstPrice();

        return std::max(global_price, local_price);
    } else { // Sell
        global_price = pos->second->getGlobalAskOrderbookFirstPrice();
        local_price = pos->second->getLocalAskOrderbookFirstPrice();

        if ((global_price && global_price < local_price) || (local_price == 0.0))
            return global_price;
        return local_price;
    }
}

bool BCDocuments::manageStockOrderBookUser(bool isRegister, const std::string& symbol, const std::string& userName) const
{
    std::lock_guard<std::mutex> guard(m_mtxStockBySymbol);

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

bool BCDocuments::manageCandleStickDataUser(bool isRegister, const std::string& userName, const std::string& symbol) const
{
    std::lock_guard<std::mutex> guard(m_mtxCandleBySymbol);

    auto pos = m_candleBySymbol.find(symbol);
    if (m_candleBySymbol.end() != pos) {
        if (isRegister)
            pos->second->registerUserInCD(userName);
        else
            pos->second->unregisterUserInCD(userName);
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

std::unordered_map<std::string, std::string> BCDocuments::getUserList()
{
    std::lock_guard<std::mutex> guard(m_mtxName2TarID);
    return m_mapName2TarID;
}

std::string BCDocuments::getTargetID(const std::string& userName)
{
    std::lock_guard<std::mutex> guard(m_mtxName2TarID);

    auto pos = m_mapName2TarID.find(userName);
    if (m_mapName2TarID.end() != pos)
        return pos->second;

    return CSTR_NULL;
}

void BCDocuments::registerUserInDoc(const std::string& targetID, const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxTarID2Name);
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2TarID);

    m_mapTarID2Name[targetID] = userName;
    m_mapName2TarID[userName] = targetID;
}

void BCDocuments::unregisterUserInDoc(const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxTarID2Name);
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2TarID);

    auto posN2ID = m_mapName2TarID.find(userName);
    if (m_mapName2TarID.end() != posN2ID) {
        const auto& tarID = posN2ID->second;
        m_mapTarID2Name.erase(tarID);

        m_mapName2TarID.erase(posN2ID);
    }
}

std::string BCDocuments::getTargetIDByUserName(const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxTarID2Name); // this is also needed to avoid potential deadlocks
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2TarID);

    auto pos = m_mapName2TarID.find(userName);
    if (m_mapName2TarID.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

std::string BCDocuments::getUserNameByTargetID(const std::string& targetID)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxTarID2Name);
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2TarID); // this is also needed to avoid potential deadlocks

    auto pos = m_mapTarID2Name.find(targetID);
    if (m_mapTarID2Name.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

void BCDocuments::addOrderbookSymbolToUser(const std::string& userName, const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxOrderbookSymbolsByName);
    m_orderbookSymbolsByName[userName].insert(symbol);
}

void BCDocuments::addCandleSymbolToUser(const std::string& userName, const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxCandleSymbolsByName);
    m_candleSymbolsByName[userName].insert(symbol);
}

void BCDocuments::removeUserFromStocks(const std::string& userName)
{
    std::lock_guard<std::mutex> guardSBS(m_mtxStockBySymbol); // Issue #3 TODO
    std::lock_guard<std::mutex> guardOSBN(m_mtxOrderbookSymbolsByName);

    auto pos = m_orderbookSymbolsByName.find(userName);
    if (m_orderbookSymbolsByName.end() != pos) {
        for (const auto& stock : pos->second) {
            m_stockBySymbol[stock]->unregisterUserInStock(userName);
        }
        m_orderbookSymbolsByName.erase(pos);
    }
}

void BCDocuments::removeUserFromCandles(const std::string& userName)
{
    std::lock_guard<std::mutex> guardCSBN(m_mtxCandleSymbolsByName);
    std::lock_guard<std::mutex> guardCBS(m_mtxCandleBySymbol);

    auto pos = m_candleSymbolsByName.find(userName);
    if (m_candleSymbolsByName.end() != pos) {
        for (const auto& stock : pos->second) {
            m_candleBySymbol[stock]->unregisterUserInCD(userName);
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

bool BCDocuments::hasSymbol(const std::string& symbol) const
{
    std::lock_guard<std::mutex> guard(m_mtxSymbols);
    return m_symbols.find(symbol) != m_symbols.end();
}

void BCDocuments::broadcastStocks() const
{
    std::lock_guard<std::mutex> guard(m_mtxStockBySymbol);

    for (const auto& i : m_stockBySymbol) {
        i.second->broadcastWholeOrderBookToAll();
    }
}

//-------------------------------------------------------------------------------------------

std::string toUpper(const std::string& str)
{
    std::string upStr;
    std::transform(str.begin(), str.end(), std::back_inserter(upStr), [](char ch) { return std::toupper(ch); });
    return upStr;
}
