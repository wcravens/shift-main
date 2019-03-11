/*
** This file contains the intiaitor which is used to
** accept/send FIX messages from/to MatchingEngine
**/

#pragma once

#include "Order.h"

#include <memory>
#include <string>

// Initiator
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketInitiator.h>

// Receiving Message Types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/SecurityList.h>

// Sending Message Types
#include <quickfix/fix50sp2/NewOrderSingle.h>

class FIXInitiator : public FIX::Application,
                     public FIX::MessageCracker // Multiple inheritance
{
public:
    static std::string s_senderID; // Sender ID
    static std::string s_targetID; // Target ID

    ~FIXInitiator() override;

    static FIXInitiator* getInstance();

    void connectMatchingEngine(const std::string& configFile, bool verbose = false);
    void disconnectMatchingEngine();

    void sendOrder(const Order& order); // Send order to ME

private:
    FIXInitiator() = default;

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void fromApp(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::ExecutionReport&, const FIX::SessionID&) override; // To receive execution report (order confirmation , trade , cancel )from matchgin engine
    void onMessage(const FIX50SP2::MarketDataIncrementalRefresh&, const FIX::SessionID&) override; // Receive order book updates
    void onMessage(const FIX50SP2::SecurityList&, const FIX::SessionID&) override; // To receive security list from ME

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Initiator> m_initiatorPtr;
};
