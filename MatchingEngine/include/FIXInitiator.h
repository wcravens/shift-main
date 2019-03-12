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

    ~FIXInitiator() override;

    static FIXInitiator* getInstance();

    void connectDatafeedEngine(const std::string& configFile);
    void disconnectDatafeedEngine();

    void sendSecurityList(const std::string& requestID, const boost::posix_time::ptime& startTime, const boost::posix_time::ptime& endTime, const std::vector<std::string>& symbols);
    void sendMarketDataRequest();

    void sendExecutionReport(action& report);
    void storeOrder(Quote& order);

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
    void onMessage(const FIX50SP2::MassQuoteAcknowledgement&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::News&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::Quote&, const FIX::SessionID&) override;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Initiator> m_initiatorPtr;
};
