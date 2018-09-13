#pragma once

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#include "BestPrice.h"
#include "CoreClient_EXPORTS.h"
#include "Order.h"
#include "OrderBookEntry.h"
#include "PortfolioItem.h"
#include "PortfolioSummary.h"

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <quickfix/Message.h>

namespace shift {
/** 
 * @brief Core class for LibCoreClient including all core functions
 */
class CORECLIENT_EXPORTS FIXInitiator;
class CORECLIENT_EXPORTS CoreClient {

    friend class FIXInitiator;

public:
    CoreClient();
    CoreClient(const std::string& username);
    virtual ~CoreClient();

    void setVerbose(bool verbose);
    bool isVerbose();
    bool isConnected();

    void setUsername(const std::string& username);
    std::string getUsername();
    std::vector<CoreClient*> getAttachedClients();

    void submitOrder(const shift::Order& order);

    // Portfolio methods
    PortfolioSummary getPortfolioSummary();
    std::map<std::string, PortfolioItem> getPortfolioItems();
    PortfolioItem getPortfolioItemBySymbol(const std::string& symbol);
    int getSubmittedOrdersSize();
    std::vector<shift::Order> getSubmittedOrders();
    int getWaitingListSize();
    std::vector<shift::Order> getWaitingList();
    void cancelAllPendingOrders();

    // Price methods
    double getOpenPriceBySymbol(const std::string& symbol);
    double getLastPriceBySymbol(const std::string& symbol);
    double getClosePriceBySymbol(const std::string& symbol, bool buy, int size);

    // Order book methods
    shift::BestPrice getBestPriceBySymbol(const std::string& symbol);
    std::vector<shift::OrderBookEntry> getOrderBookBySymbolAndType(const std::string& symbol, char type);
    std::vector<shift::OrderBookEntry> getOrderBookWithDestBySymbolAndType(const std::string& symbol, char type);

    // Symbols list and company names
    std::vector<std::string> getStockList();
    void requestCompanyNames();
    std::map<std::string, std::string> getCompanyNames();
    std::string getCompanyNameBySymbol(const std::string& symbol);

    // Sample prices
    bool requestSamplePrices(std::vector<std::string> symbols, double samplingFrequency = 1, unsigned int samplingWindow = 31);
    bool cancelSamplePricesRequest(const std::vector<std::string>& symbols);
    bool cancelAllSamplePricesRequests();
    int getSamplePricesSizeBySymbol(const std::string& symbol);
    std::list<double> getSamplePricesBySymbol(const std::string& symbol, bool midPrices = false);
    int getLogReturnsSizeBySymbol(const std::string& symbol);
    std::list<double> getLogReturnsBySymbol(const std::string& symbol, bool midPrices = false);

    // Subscription methods
    bool subOrderBook(const std::string& symbol);
    bool unsubOrderBook(const std::string& symbol);
    bool subAllOrderBook();
    bool unsubAllOrderBook();
    std::vector<std::string> getSubscribedOrderBookList();
    bool subCandleData(const std::string& symbol);
    bool unsubCandleData(const std::string& symbol);
    bool subAllCandleData();
    bool unsubAllCandleData();
    std::vector<std::string> getSubscribedCandlestickList();

protected:
    // Inline methods
    void debugDump(const std::string& message);

    // FIXInitiator interface
    bool attach(FIXInitiator& initiator);
    void storePortfolio(double totalRealizedPL, double totalBuyingPower, int totalShares, std::string symbol, int shares, double price, double realizedPL);
    void storeWaitingList(const std::vector<shift::Order>& waitingList);

    // FIXInitiator callback methods
    virtual void receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp){};
    virtual void receiveLastPrice(const std::string& symbol){};
    virtual void receivePortfolio(const std::string& symbol){};
    virtual void receiveWaitingList(){};

    // Sample prices
    void calculateSamplePrices(std::vector<std::string> symbols, double samplingFrequency, unsigned int samplingWindow);

private:
    FIXInitiator* m_fixInitiator;
    std::string m_username;
    bool m_verbose;

    std::mutex m_mutex_portfolioSummary;
    std::mutex m_mutex_symbol_portfolioItem; //!< Mutex for the <Symbol, PortfolioItem> map.
    std::mutex m_mutex_submittedOrders;
    std::mutex m_mutex_waitingList;
    std::mutex m_mutex_samplePricesFlags;
    std::mutex m_mutex_samplePrices;

    PortfolioSummary m_portfolioSummary;
    std::map<std::string, PortfolioItem> m_symbol_portfolioItem;
    std::vector<std::string> m_submittedOrdersIDs;
    std::unordered_map<std::string, shift::Order> m_submittedOrders;
    std::vector<shift::Order> m_waitingList;
    std::unordered_map<std::string, bool> m_samplePricesFlags;
    std::unordered_map<std::string, std::list<double>> m_sampleLastPrices;
    std::unordered_map<std::string, std::list<double>> m_sampleMidPrices;
};

} // shift
