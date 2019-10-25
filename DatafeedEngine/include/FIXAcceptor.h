#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// acceptor
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/MessageCracker.h>
// #include <quickfix/NullStore.h>
#include <quickfix/SocketAcceptor.h>

#define HAVE_POSTGRESQL false
#if HAVE_POSTGRESQL
#include <quickfix/PostgreSQLLog.h>
#include <quickfix/PostgreSQLStore.h>
#endif

// receiving message types
#include <quickfix/fix50sp2/ExecutionReport.h>
#include <quickfix/fix50sp2/MarketDataRequest.h>
#include <quickfix/fix50sp2/NewOrderSingle.h>
#include <quickfix/fix50sp2/SecurityList.h>

// sending message yypes
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

    static FIXAcceptor* getInstance();

    bool connectMatchingEngine(const std::string& configFile, bool verbose = false, const std::string& cryptoKey = "", const std::string& dbConfigFile = "");
    void disconnectMatchingEngine();

    static double s_decimalRound(double value, int precision);

    static void sendNotice(const std::string& targetID, const std::string& requestID, const std::string& text);
    static void sendRawData(const std::string& targetID, const std::vector<RawData>& rawData);

private:
    FIXAcceptor();
    ~FIXAcceptor() override;

    FIXAcceptor(const FIXAcceptor&) = delete;
    void operator=(const FIXAcceptor&) = delete;

    // QuickFIX methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    void fromApp(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override;
    void onMessage(const FIX50SP2::SecurityList&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::MarketDataRequest&, const FIX::SessionID&) override;
    void onMessage(const FIX50SP2::NewOrderSingle&, const FIX::SessionID&) override;

    // DO NOT change order of these unique_ptrs:
    std::unique_ptr<FIX::LogFactory> m_logFactoryPtr;
    std::unique_ptr<FIX::MessageStoreFactory> m_messageStoreFactoryPtr;
    std::unique_ptr<FIX::Acceptor> m_acceptorPtr; ///> Underlying FIX protocal acceptor instance.

    std::mutex m_mtxReqsProcs; ///> To guard m_requestsProcessorByTarget.
    std::unordered_map<std::string /*Target ID*/, std::unique_ptr<RequestsProcessorPerTarget>> m_requestsProcessorByTarget; ///> Collection of currently running Requests Processors for their targets. Each unique target (distinguished by Target ID) has its own independant processor.
};
