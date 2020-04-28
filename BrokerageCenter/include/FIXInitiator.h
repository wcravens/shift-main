#pragma once

#include "Order.h"

#include <memory>
#include <string>

// initiator
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
// #include <quickfix/NullStore.h>
#include <quickfix/SocketInitiator.h>

#define HAVE_POSTGRESQL false
#if HAVE_POSTGRESQL
#include <quickfix/PostgreSQLLog.h>
#include <quickfix/PostgreSQLStore.h>
#endif

// receiving message types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix50sp2/SecurityList.h>

// sending message types
#include <quickfix/fix50sp2/NewOrderSingle.h>

class FIXInitiator : public FIX::Application,
                     public FIX::MessageCracker {
public:
    static std::string s_senderID;
    static std::string s_targetID;

    ~FIXInitiator() override;

    static auto getInstance() -> FIXInitiator&;

    auto connectMatchingEngine(const std::string& configFile, bool verbose = false, const std::string& cryptoKey = "", const std::string& dbConfigFile = "") -> bool;
    void disconnectMatchingEngine();

    static void s_sendOrder(const Order& order); // send order to Matching Engine

private:
    FIXInitiator() = default; // singleton pattern
    FIXInitiator(const FIXInitiator&) = delete; // forbid copying
    auto operator=(const FIXInitiator&) -> FIXInitiator& = delete; // forbid assigning

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) noexcept(false) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) noexcept(false) override {}
    void fromApp(const FIX::Message&, const FIX::SessionID&) noexcept(false) override;
    void onMessage(const FIX50SP2::SecurityList&, const FIX::SessionID&) override; // to receive security list from Matching Engine
    void onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh&, const FIX::SessionID&) override; // to receive complete order book from Matching Engine
    void onMessage(const FIX50SP2::MarketDataIncrementalRefresh&, const FIX::SessionID&) override; // to receive order book updates from Matching Engine
    void onMessage(const FIX50SP2::ExecutionReport&, const FIX::SessionID&) override; // to receive execution report (order confirmation, trade, cancel) from Matching Engine

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Initiator> m_initiatorPtr;
};
