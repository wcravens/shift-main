#pragma once

#include <memory>
#include <string>
#include <unordered_map>

// Acceptor
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/NullStore.h>
#include <quickfix/SocketAcceptor.h>

// Receiving Message Types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/SecurityList.h>

// Sending Message Types
#include <quickfix/fix50sp2/News.h>
#include <quickfix/fix50sp2/Quote.h>

class RequestsProcessorPerTarget;
struct RawData;

/**
 * @brief The acceptor for Matching Engine using FIX protocal.
 *        Handles incoming messages from ME, and/or sends messages back.
 *        Encapsulates all data conversion between FIX types and C++ standard types.
 */
class FIXAcceptor : public FIX::Application,
                    public FIX::MessageCracker {
public:
    static std::string s_senderID;

    ~FIXAcceptor() override;

    static FIXAcceptor* getInstance();

    void connectMatchingEngine(const std::string& configFile, bool verbose = false);
    void disconnectMatchingEngine();

    static void sendRawData(const RawData& rawData, const std::string& targetID);
    static void sendNotice(const std::string& text, const std::string& requestID, const std::string& targetID);

private:
    FIXAcceptor();

    // QuickFIX methods
    void onCreate(const FIX::SessionID& sessionID) override;
    void onLogon(const FIX::SessionID& sessionID) override;
    void onLogout(const FIX::SessionID& sessionID) override;
    void toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) override {}
    void toApp(FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    // Receive execution report and save it into database
    void onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::MarketDataRequest& message, const FIX::SessionID& sessionID) override;
    void onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) override;

    // Do NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr; ///> Underlying FIX protocal acceptor instance

    std::unordered_map<std::string /*Target ID*/, std::unique_ptr<RequestsProcessorPerTarget>> m_requestsProcessors; ///> Collection of currently running Requests Processors for their targets. Each unique target (distinguished by Target ID) has its own independant processor.
};
