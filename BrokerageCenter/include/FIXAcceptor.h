#pragma once

#include "CandlestickDataPoint.h"
#include "ExecutionReport.h"
#include "Order.h"
#include "OrderBookEntry.h"
#include "PortfolioItem.h"
#include "PortfolioSummary.h"
#include "Transaction.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// acceptor
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
// #include <quickfix/NullStore.h>
#include <quickfix/SocketAcceptor.h>

#define HAVE_POSTGRESQL false
#if HAVE_POSTGRESQL
#include <quickfix/PostgreSQLLog.h>
#include <quickfix/PostgreSQLStore.h>
#endif

// receiving message types
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/RFQRequest.h>
#include <quickfix/fix50sp2/UserRequest.h>
#include <quickfix/fixt11/Logon.h>

// sending message types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix50sp2/NewOrderList.h>
#include <quickfix/fix50sp2/PositionReport.h>
#include <quickfix/fix50sp2/SecurityList.h>

class FIXAcceptor : public FIX::Application, public FIX::MessageCracker {
public:
    static std::string s_senderID;

    ~FIXAcceptor() override;

    static auto getInstance() -> FIXAcceptor&;

    auto connectClients(const std::string& configFile, bool verbose = false, const std::string& cryptoKey = "", const std::string& dbConfigFile = "") -> bool;
    void disconnectClients();

    static void s_sendLastPrice2All(const Transaction& transac);
    static void s_sendOrderBook(const std::vector<std::string>& targetList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBook);
    static void s_sendOrderBookUpdate(const std::vector<std::string>& targetList, const OrderBookEntry& update);
    static void s_sendCandlestickData(const std::vector<std::string>& targetList, const CandlestickDataPoint& cdPoint);

    static void s_sendConfirmationReport(const ExecutionReport& report);
    static void s_sendPortfolioSummary(const std::string& userID, const PortfolioSummary& summary);
    static void s_sendPortfolioItem(const std::string& userID, const PortfolioItem& item);
    static void s_sendWaitingList(const std::string& userID, const std::unordered_map<std::string, Order>& orders);

private:
    FIXAcceptor() = default; // singleton pattern
    FIXAcceptor(const FIXAcceptor&) = delete; // forbid copying
    auto operator=(const FIXAcceptor&) -> FIXAcceptor& = delete; // forbid assigning

    static void s_sendSecurityList(const std::string& targetID, const std::unordered_set<std::string>& symbols);
    static void s_sendUserIDResponse(const std::string& targetID, const std::string& userReqID, const std::string& username, const std::string& userID);

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override { }
    void toApp(FIX::Message&, const FIX::SessionID&) noexcept(false) override { }
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) noexcept(false) override;
    void fromApp(const FIX::Message&, const FIX::SessionID&) noexcept(false) override;
    void onMessage(const FIX50SP2::UserRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::MarketDataRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::RFQRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::NewOrderSingle&, const FIX::SessionID&) override;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr;
};
