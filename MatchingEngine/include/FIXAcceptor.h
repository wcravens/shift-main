/*
** Connector to BrokerageCenter
**/

#pragma once

#include "Action.h"
#include "Newbook.h"
#include "QuoteConfirm.h"

#include <mutex>
#include <set>
#include <string>
#include <unordered_set>

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
private:
    static std::string s_senderID;

    std::set<std::string> m_symbols;

    // Do NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr;

    mutable std::mutex m_mtxTargetList;
    std::unordered_set<std::string> m_targetList;

private:
    FIXAcceptor() = default;

    void sendSecurityList(const std::string& targetID);
    void sendOrderConfirmation(const std::string& targetID, const QuoteConfirm& confirmation);

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override {}
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void fromApp(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::NewOrderSingle&, const FIX::SessionID&) override;

public:
    ~FIXAcceptor() override;

    static FIXAcceptor* getInstance();

    void addSymbol(const std::string& symbol);
    const std::set<std::string>& getSymbols() const;

    void connectBrokerageCenter(const std::string& configFile);
    void disconnectBrokerageCenter();

    void sendOrderBookUpdate2All(Newbook& update);
    void sendExecutionReport2All(Action& report);
};
