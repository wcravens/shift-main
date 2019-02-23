/*
** Connector to BrokerageCenter
**/

#pragma once

#include "Newbook.h"
#include "QuoteConfirm.h"
#include "action.h"

#include <list>
#include <mutex>
#include <string>

// Acceptor
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketAcceptor.h>

// Receiving Message Types
#include <quickfix/fix50sp2/NewOrderSingle.h>

// Sending Message Types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/SecurityList.h>

class FIXAcceptor
    : public FIX::Application,
      public FIX::MessageCracker {
public:
    static std::string s_senderID;

    FIXAcceptor() = default;
    ~FIXAcceptor() override;

    void connectBrokerageCenter(const std::string& configFile);
    void disconnectBrokerageCenter();

    void sendSecurityList(const std::string& clientID); // send security list from ME to BC

    static void SendOrderBookUpdate(std::string& clientID, Newbook& newbook); // Send order book update to client
    static void SendOrderBookUpdate2All(Newbook& newbook); // Send order book update to all client
    static void SendExecution2Client(std::string& targetID, action& actions);
    static void SendExecution(const action& actions);

    std::list<std::string> symbollist;

private:
    static std::list<std::string> clientlist;
    static std::mutex clientid_mutex;
    static std::mutex ExecReport_mutex;

    // QuickFIX methods
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override {}
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {}
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) override;

    void sendQuoteConfirmation(QuoteConfirm& confirm);

    // Do NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr;
};
