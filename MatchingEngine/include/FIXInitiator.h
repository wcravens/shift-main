#pragma once

#include "ExecutionReport.h"
#include "Order.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

// initiator
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketInitiator.h>

// receiving message types
#include <quickfix/fix50sp2/MassQuoteAcknowledgement.h>
#include <quickfix/fix50sp2/News.h>
#include <quickfix/fix50sp2/Quote.h>

// sending message types
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

    bool connectDatafeedEngine(const std::string& configFile);
    void disconnectDatafeedEngine();

    bool sendSecurityListRequestAwait(const std::string& requestID, const boost::posix_time::ptime& startTime, const boost::posix_time::ptime& endTime, const std::vector<std::string>& symbols, int numSecondsPerDataChunk);
    void sendNextDataRequest();

    void storeOrder(const Order& order);

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
    void onMessage(const FIX50SP2::News&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::Quote&, const FIX::SessionID&) override;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Initiator> m_initiatorPtr;

    std::condition_variable m_cvMarketDataReady;
    mutable std::mutex m_mtxMarketDataReady;
    std::string m_lastMarketDataRequestID;
};
