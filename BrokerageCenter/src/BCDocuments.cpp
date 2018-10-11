#include "BCDocuments.h"
#include "DBConnector.h"

#include <algorithm>

/*static*/ BCDocuments* BCDocuments::instance()
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
        candPtr.reset(new StockSummary(transac.symbol, transac.price, transac.price, transac.price, transac.price, transac.price, StockSummary::s_toUnixTimestamp(transac.execTime)));
        candPtr->spawn();
    }
    candPtr->enqueueTransaction(transac);
}

void BCDocuments::addRiskManagementToClient(const std::string& userName, double buyingPower, int shares, double price, const std::string& symbol)
{ //for low frequency client use
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);

    auto res = m_riskManagementByName.emplace(userName, nullptr);
    if (res.second) {
        auto& rmPtr = res.first->second;
        rmPtr.reset(new RiskManagement(userName, buyingPower, shares));
        rmPtr->insertPortfolioItem(symbol, { symbol, price, shares });
        rmPtr->spawn();
    }
}

auto BCDocuments::addRiskManagementToClientNoLock(const std::string& userName) -> decltype(m_riskManagementByName)::iterator
{
    auto res = m_riskManagementByName.emplace(userName, nullptr);
    if (res.second) {
        auto& rmPtr = res.first->second;
        
        auto summary = DBConnector::s_readFieldsOfRow(
            "SELECT buying_power, holding_balance, borrowed_balance, total_pl, total_shares\n"
            "FROM web_traders INNER JOIN portfolio_summary ON web_traders.id = portfolio_summary.client_id\n"
            "WHERE username = '" + userName + "';"
            , 5
        );
        
        if(summary.empty())
            rmPtr.reset(new RiskManagement(userName, 1e5));
        else
            rmPtr.reset(new RiskManagement(userName, std::stod(summary[0]), std::stod(summary[1]), std::stod(summary[2]), std::stod(summary[3]), std::stoi(summary[4])));

        rmPtr->spawn();
    }
    return res.first;
}

void BCDocuments::addQuoteToClientRiskManagement(const std::string& userName, const Quote& quote)
{
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);
    addRiskManagementToClientNoLock(userName);
    m_riskManagementByName[userName]->enqueueQuote(quote);
}

void BCDocuments::addReportToClientRiskManagement(const std::string& userName, const Report& report)
{
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);
    if ("TR" == userName)
        return;

    m_riskManagementByName[userName]->enqueueExecRpt(report);
}

double BCDocuments::getStockOrderBookMarketFirstPrice(bool isBuy, const std::string& symbol) const
{
    double global_price = 0.0;
    double local_price = 0.0;
    std::lock_guard<std::mutex> guard(m_mtxStockBySymbol);

    auto pos = m_stockBySymbol.find(symbol);
    if (m_stockBySymbol.end() == pos) {
        return 0.0;
    } else {
        if (isBuy) {
            global_price = pos->second->getGlobalBidOrderbookFirstPrice();
            local_price = pos->second->getLocalBidOrderbookFirstPrice();

            if (global_price > local_price)
                return global_price;
            else
                return local_price;
        } else { // Sell
            global_price = pos->second->getGlobalAskOrderbookFirstPrice();
            local_price = pos->second->getLocalAskOrderbookFirstPrice();

            if ((global_price && global_price < local_price) || (local_price == 0.0))
                return global_price;
            else
                return local_price;
        }
    }
}

bool BCDocuments::manageStockOrderBookClient(bool isRegister, const std::string& symbol, const std::string& userName) const
{
    std::lock_guard<std::mutex> guard(m_mtxStockBySymbol);

    auto pos = m_stockBySymbol.find(symbol);
    if (m_stockBySymbol.end() != pos) {
        if (isRegister)
            pos->second->registerClientInStock(userName);
        else // unregister
            pos->second->unregisterClientInStock(userName);
        return true;
    }

    return false;
}

bool BCDocuments::manageCandleStickDataClient(bool isRegister, const std::string& userName, const std::string& symbol) const
{
    std::lock_guard<std::mutex> guard(m_mtxCandleBySymbol);

    auto pos = m_candleBySymbol.find(symbol);
    if (m_candleBySymbol.end() != pos) {
        if (isRegister)
            pos->second->registerClientInSS(userName);
        else
            pos->second->unregisterClientInSS(userName);
        return true;
    }

    return false;
}

int BCDocuments::sendHistoryToClient(const std::string& userName)
{
    int res = 0;
    std::lock_guard<std::mutex> guard(m_mtxRiskManagementByName);

    auto pos = m_riskManagementByName.find(userName);
    if (m_riskManagementByName.end() == pos) {
        pos = addRiskManagementToClientNoLock(userName);
        res = 1;
    }
    pos->second->sendPortfolioHistory();
    pos->second->sendQuoteHistory();

    return res;
}

std::unordered_map<std::string, std::string> BCDocuments::getClientList()
{
    std::lock_guard<std::mutex> guard(m_mtxName2ID);
    return m_mapName2ID;
}

std::string BCDocuments::getClientID(const std::string& name)
{
    std::lock_guard<std::mutex> guard(m_mtxName2ID);

    auto pos = m_mapName2ID.find(name);
    if (m_mapName2ID.end() != pos)
        return pos->second;

    return CSTR_NULL;
}

void BCDocuments::registerClient(const std::string& clientID, const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxID2Name);
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2ID);

    m_mapID2Name[clientID] = userName;
    m_mapName2ID[userName] = clientID;
}

void BCDocuments::unregisterClientInDocs(const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxID2Name);
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2ID);

    auto posN2ID = m_mapName2ID.find(userName);
    if (m_mapName2ID.end() != posN2ID) {
        m_mapID2Name.erase(posN2ID->second);
        m_mapName2ID.erase(posN2ID);
    }
}

std::string BCDocuments::getClientIDByName(const std::string& userName)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxID2Name); // this is also needed to avoid potential deadlocks
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2ID);

    auto pos = m_mapName2ID.find(userName);
    if (m_mapName2ID.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

std::string BCDocuments::getClientNameByID(const std::string& clientID)
{
    std::lock_guard<std::mutex> guardID2N(m_mtxID2Name);
    std::lock_guard<std::mutex> guardN2ID(m_mtxName2ID); // this is also needed to avoid potential deadlocks

    auto pos = m_mapID2Name.find(clientID);
    if (m_mapID2Name.end() == pos)
        return ::STDSTR_NULL;

    return pos->second;
}

void BCDocuments::addOrderbookSymbolToClient(const std::string& userName, const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxOrderbookSymbolsByName);
    m_orderbookSymbolsByName[userName].insert(symbol);
}

void BCDocuments::addCandleSymbolToClient(const std::string& userName, const std::string& symbol)
{
    std::lock_guard<std::mutex> guard(m_mtxCandleSymbolsByName);
    m_candleSymbolsByName[userName].insert(symbol);
}

void BCDocuments::removeClientFromStocks(const std::string& userName)
{
    std::lock_guard<std::mutex> guardSBS(m_mtxStockBySymbol);
    std::lock_guard<std::mutex> guardOSBN(m_mtxOrderbookSymbolsByName);

    auto pos = m_orderbookSymbolsByName.find(userName);
    if (m_orderbookSymbolsByName.end() != pos) {
        for (const auto& stock : pos->second) {
            m_stockBySymbol[stock]->unregisterClientInStock(userName);
        }
        m_orderbookSymbolsByName.erase(pos);
    }
}

void BCDocuments::removeClientFromCandles(const std::string& userName)
{
    std::lock_guard<std::mutex> guardCSBN(m_mtxCandleSymbolsByName);
    std::lock_guard<std::mutex> guardCBS(m_mtxCandleBySymbol);

    auto pos = m_candleSymbolsByName.find(userName);
    if (m_candleSymbolsByName.end() != pos) {
        for (const auto& stock : pos->second) {
            m_candleBySymbol[stock]->unregisterClientInSS(userName);
        }
        m_candleSymbolsByName.erase(pos);
    }
}

void BCDocuments::removeCandleSymbolFromClient(const std::string& userName, const std::string& symbol)
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

std::string toUpper(const std::string& str)
{
    std::string upStr;
    std::transform(str.begin(), str.end(), std::back_inserter(upStr), [](char ch) { return std::toupper(ch); });
    return upStr;
}
