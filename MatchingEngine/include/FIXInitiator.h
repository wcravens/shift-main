/* 
** Connnector to DatafeedEngine
**/

#pragma once

#include "Quote.h"
#include "action.h"

#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

// Initiator
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketInitiator.h>

// Receiving Message Types
#include <quickfix/fix50sp2/MassQuoteAcknowledgement.h>
#include <quickfix/fix50sp2/News.h>
#include <quickfix/fix50sp2/Quote.h>

// Sending Message Types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/SecurityList.h>

class FIXInitiator : public FIX::Application,
                     public FIX::MessageCracker {
public:
    static std::string s_senderID;
    static std::string s_targetID;

    FIXInitiator() = default;
    ~FIXInitiator() override;

    void connectDatafeedEngine(const std::string& configFile);
    void disconnectDatafeedEngine();

    void sendSecurityList(const std::string& request_id, const boost::posix_time::ptime& start_time, const boost::posix_time::ptime& end_time, const std::vector<std::string>& symbols);
    void sendMarketDataRequest();

    static void SendActionRecord(action& actions);
    static void StoreOrder(Quote& quote);

private:
    // QuickFIX methods
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {}
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override;
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::MassQuoteAcknowledgement& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::News& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::Quote& message, const FIX::SessionID& sessionID) override;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Initiator> m_initiatorPtr;
};
