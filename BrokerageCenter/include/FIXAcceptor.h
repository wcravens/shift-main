/*
** This file contains the acceptor which is used to
** accept/send FIX messages from/to CoreClient
**/

#pragma once

#include "OrderBookEntry.h"
#include "PortfolioItem.h"
#include "PortfolioSummary.h"
#include "Quote.h"
#include "Report.h"
#include "TempCandlestickData.h"
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
    static std::string s_senderID; // Sender ID

    ~FIXAcceptor() override;

    static FIXAcceptor* getInstance();

    void connectClientComputers(const std::string& configFile, bool verbose = false);
    void disconnectClientComputers();

    void sendPortfolioSummary(const std::string& userName, const PortfolioSummary& summary);
    void sendPortfolioItem(const std::string& userName, const PortfolioItem& item);
    void sendQuoteHistory(const std::string& userName, const std::unordered_map<std::string, Quote>& quotes);
    void sendOrderbookUpdate(const std::string& userName, const OrderBookEntry& update); // send order book update to user
    void sendOrderbookUpdate2All(const std::unordered_set<std::string>& userList, const OrderBookEntry& newOrderBook);
    void sendNewBook2all(const std::unordered_set<std::string>& userList, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName);
    void sendConfirmationReport(const Report& report); // send quote conformation report
    void sendOrderBook(const std::string& userName, const std::map<double, std::map<std::string, OrderBookEntry>>& orderBookName);
    void sendTempCandlestickData(const std::string& userName, const TempCandlestickData& tmpCandle); // for candlestick data
    void sendLatestStockPrice(const std::string& userName, const Transaction& transac);
    void sendLatestStockPrice2All(const Transaction& transac);

    bool subscribeOrderBook(const std::string& userName, const std::string& symbol);
    bool unsubscribeOrderBook(const std::string& userName, const std::string& symbol);
    bool subscribeCandleData(const std::string& userName, const std::string& symbol);
    bool unsubscribeCandleData(const std::string& userName, const std::string& symbol);

private:
    FIXAcceptor() = default;

    // QuickFIX methods
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {}
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override;
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::RFQRequest& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::UserRequest& message, const FIX::SessionID& sessionID) override;

    void sendSymbols(const std::string& targetID, const std::unordered_set<std::string>& symbols);

    // Do NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr;
};
