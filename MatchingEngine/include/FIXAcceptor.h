#pragma once

#include "ExecutionReport.h"
#include "OrderBookEntry.h"
#include "OrderConfirmation.h"

#include <mutex>
#include <set>
#include <string>
#include <unordered_set>

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
#include <quickfix/fix50sp2/NewOrderSingle.h>

// sending message types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix50sp2/SecurityList.h>

class FIXAcceptor
    : public FIX::Application,
      public FIX::MessageCracker {
public:
    static std::string s_senderID;

    static FIXAcceptor* getInstance();

    bool connectBrokerageCenter(const std::string& configFile, bool verbose, const std::string& cryptoKey = "", const std::string& dbConfigFile = "");
    void disconnectBrokerageCenter();

    void sendOrderBook(const std::string& targetID, const std::vector<OrderBookEntry>& orderBook);
    void sendOrderBookUpdates(const std::vector<OrderBookEntry>& orderBookUpdates);
    void sendExecutionReports(const std::vector<ExecutionReport>& executionReports);

private:
    FIXAcceptor() = default;
    ~FIXAcceptor() override;

    FIXAcceptor(const FIXAcceptor&) = delete;
    void operator=(const FIXAcceptor&) = delete;

    void sendSecurityList(const std::string& targetID);
    void sendOrderConfirmation(const std::string& targetID, const OrderConfirmation& confirmation);

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) noexcept(false) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) noexcept(false) override {}
    void fromApp(const FIX::Message&, const FIX::SessionID&) noexcept(false) override;
    void onMessage(const FIX50SP2::NewOrderSingle&, const FIX::SessionID&) override;

    mutable std::mutex m_mtxTargetSet;
    std::unordered_set<std::string> m_targetSet;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr;
};
