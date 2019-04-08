/*
** This file contains the acceptor which is used to
** accept/send FIX messages from/to CoreClient
**/

#pragma once

#include "CandlestickDataPoint.h"
#include "Order.h"
#include "OrderBookEntry.h"
#include "PortfolioItem.h"
#include "PortfolioSummary.h"
#include "Report.h"
#include "Transaction.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Acceptor
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketAcceptor.h>

// Receiving Message Types
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/RFQRequest.h>
#include <quickfix/fix50sp2/UserRequest.h>
#include <quickfix/fixt11/Logon.h>

// Sending Message Types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataIncrementalRefresh.h>
#include <quickfix/fix50sp2/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix50sp2/MassQuoteAcknowledgement.h>
#include <quickfix/fix50sp2/PositionReport.h>
#include <quickfix/fix50sp2/SecurityList.h>

class FIXAcceptor : public FIX::Application,
                    public FIX::MessageCracker {
public:
    static std::string s_senderID;

    ~FIXAcceptor() override;

    static FIXAcceptor* getInstance();

    void connectClients(const std::string& configFile, bool verbose = false);
    void disconnectClients();

    void sendLastPrice2All(const Transaction& transac);
    void sendOrderBook(const std::vector<std::string>& targetList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName);
    static void s_setAddGroupIntoMarketDataMsg(FIX::Message& message, FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries& entryGroup, const OrderBookEntry& entry);
    void sendOrderBookUpdate(const std::vector<std::string>& targetList, const OrderBookEntry& update);
    void sendCandlestickData(const std::vector<std::string>& targetList, const CandlestickDataPoint& cdPoint);

    void sendConfirmationReport(const Report& report);
    void sendPortfolioSummary(const std::string& username, const PortfolioSummary& summary);
    void sendPortfolioItem(const std::string& username, const PortfolioItem& item);
    void sendWaitingList(const std::string& username, const std::unordered_map<std::string, Order>& orders);

private:
    FIXAcceptor() = default;

    void sendSecurityList(const std::string& targetID, const std::unordered_set<std::string>& symbols);
    void sendUserIDResponse(const std::string& targetID, const std::string& username, const std::string& userID);

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    void fromApp(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::UserRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::MarketDataRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::RFQRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::NewOrderSingle&, const FIX::SessionID&) override;

    // Do NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr;
};
