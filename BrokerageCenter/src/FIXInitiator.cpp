#include "FIXInitiator.h"

#include "BCDocuments.h"
#include "DBConnector.h"
#include "FIXAcceptor.h"

#include <atomic>
#include <cassert>

#include <shift/miscutils/fix/HelperFunctions.h>
#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

/* static */ std::string FIXInitiator::s_senderID;
/* static */ std::string FIXInitiator::s_targetID;

// predefined constant FIX message fields (to avoid recalculations):
static const auto& FIXFIELD_BEGINSTRING_FIXT11 = FIX::BeginString(FIX::BeginString_FIXT11);
static const auto& FIXFIELD_PARTYROLE_CLIENTID = FIX::PartyRole(FIX::PartyRole_CLIENT_ID);

FIXInitiator::~FIXInitiator() // override
{
    disconnectMatchingEngine();
}

/*static*/ FIXInitiator* FIXInitiator::getInstance()
{
    static FIXInitiator s_FIXInitInst;
    return &s_FIXInitInst;
}

void FIXInitiator::connectMatchingEngine(const std::string& configFile, bool verbose)
{
    disconnectMatchingEngine();

    FIX::SessionSettings settings(configFile);
    const FIX::Dictionary& commonDict = settings.get();

    if (commonDict.has("FileLogPath")) { // store all log events into flat files
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
#if HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLLogDatabase")) { // store all log events into database
        m_logFactoryPtr.reset(new FIX::PostgreSQLLogFactory(settings));
#endif
    } else { // display all log events onto the standard output
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, verbose)); // incoming, outgoing, event
    }

    if (commonDict.has("FileStorePath")) { // store all outgoing messages into flat files
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
#if HAVE_POSTGRESQL
    } else if (commonDict.has("PostgreSQLStoreDatabase")) { // store all outgoing messages into database
        m_messageStoreFactoryPtr.reset(new FIX::PostgreSQLStoreFactory(settings));
#endif
    } else { // store all outgoing messages in memory
        m_messageStoreFactoryPtr.reset(new FIX::MemoryStoreFactory());
    }
    // } else { // do not store messages
    //     m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    // }

    m_initiatorPtr.reset(new FIX::SocketInitiator(*getInstance(), *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Initiator is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_initiatorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
    }
}

void FIXInitiator::disconnectMatchingEngine()
{
    if (!m_initiatorPtr)
        return;

    cout << '\n'
         << COLOR "Initiator is stopping..." NO_COLOR << '\n'
         << flush;

    m_initiatorPtr->stop();
    m_initiatorPtr = nullptr;
    m_messageStoreFactoryPtr = nullptr;
    m_logFactoryPtr = nullptr;
}

/**
 * @brief Sending the order to the server.
 */
void FIXInitiator::sendOrder(const Order& order)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGINSTRING_FIXT11);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getID()));
    message.setField(FIX::Symbol(order.getSymbol()));
    message.setField(FIX::Side(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(order.getSize()));
    message.setField(FIX::OrdType(order.getType())); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(order.getPrice()));

    shift::fix::addFIXGroup<FIX50SP2::NewOrderSingle::NoPartyIDs>(message,
        ::FIXFIELD_PARTYROLE_CLIENTID,
        FIX::PartyID(order.getUserID()));

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method called when a new Session was created. Set Sender and Target Comp ID.
 */
void FIXInitiator::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID().getValue();
    s_targetID = sessionID.getTargetCompID().getValue();
    // cout << "SenderID: " << s_senderID << "\n"
    //      << "TargetID: " << s_targetID << endl;
}

void FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logon - " << sessionID.toString() << endl;
}

void FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logout - " << sessionID.toString() << endl;
}

void FIXInitiator::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID); // message is message type
}

/*
 * @brief Receive security list from ME.
 */
void FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID&) // override
{
    // this test is required because if there is a disconnection between ME and BC,
    // the ME will send the security list again during the reconnection procedure
    if (BCDocuments::s_isSecurityListReady)
        return;

    FIX::NoRelatedSym numOfGroups;
    message.getField(numOfGroups);
    if (numOfGroups.getValue() < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    FIX::Symbol symbol;

    auto* docs = BCDocuments::getInstance();
    for (int i = 1; i <= numOfGroups.getValue(); i++) {
        message.getGroup(static_cast<unsigned int>(i), relatedSymGroup);
        relatedSymGroup.getField(symbol);

        docs->addSymbol(symbol.getValue());
        docs->attachOrderBookToSymbol(symbol.getValue());
        docs->attachCandlestickDataToSymbol(symbol.getValue());
    }

    // now, it's safe to advance all routines that *read* permanent data structures created above:
    BCDocuments::s_isSecurityListReady = true;
}

/**
 * @brief Receive complete order book
 */
void FIXInitiator::onMessage(const FIX50SP2::MarketDataSnapshotFullRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.getField(numOfEntries);
    if (numOfEntries.getValue() < 1) {
        cout << "Cannot find the Entries group in MarketDataSnapshotFullRefresh!" << endl;
        return;
    }

    static FIX::Symbol symbol;

    static FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries entryGroup;
    static FIX::MDEntryType bookType;
    static FIX::MDEntryPx price;
    static FIX::MDEntrySize size;
    static FIX::MDEntryDate simulationDate;
    static FIX::MDEntryTime simulationTime;
    static FIX::MDMkt destination;

    // #pragma GCC diagnostic ignored ....

    FIX::Symbol* pSymbol;

    FIX50SP2::MarketDataSnapshotFullRefresh::NoMDEntries* pEntryGroup;
    FIX::MDEntryType* pBookType;
    FIX::MDEntryPx* pPrice;
    FIX::MDEntrySize* pSize;
    FIX::MDEntryDate* pSimulationDate;
    FIX::MDEntryTime* pSimulationTime;
    FIX::MDMkt* pDestination;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pSymbol = &symbol;
        pEntryGroup = &entryGroup;
        pBookType = &bookType;
        pPrice = &price;
        pSize = &size;
        pSimulationDate = &simulationDate;
        pSimulationTime = &simulationTime;
        pDestination = &destination;
    } else { // > 1 threads; always safe way:
        pSymbol = new decltype(symbol);
        pEntryGroup = new decltype(entryGroup);
        pBookType = new decltype(bookType);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pSimulationDate = new decltype(simulationDate);
        pSimulationTime = new decltype(simulationTime);
        pDestination = new decltype(destination);
    }

    message.getField(*pSymbol);

    for (int i = 1; i <= numOfEntries.getValue(); i++) {
        message.getGroup(static_cast<unsigned int>(i), *pEntryGroup);

        pEntryGroup->getField(*pBookType);
        pEntryGroup->getField(*pPrice);
        pEntryGroup->getField(*pSize);
        pEntryGroup->getField(*pSimulationDate);
        pEntryGroup->getField(*pSimulationTime);
        pEntryGroup->getField(*pDestination);

        OrderBookEntry entry{
            static_cast<OrderBookEntry::Type>(pBookType->getValue()),
            pSymbol->getValue(),
            pPrice->getValue(),
            static_cast<int>(pSize->getValue()),
            pDestination->getValue(),
            pSimulationDate->getValue(),
            pSimulationTime->getValue()
        };

        // - the first entry is guaranteed to have price <= 0.0: this will tell the
        // order book object that the targeted order book type must first be cleared.
        // - the following entries will be in such order so that the standard update
        // procedure for a given order book type may be used without information loss
        BCDocuments::getInstance()->onNewOBUpdateForOrderBook(pSymbol->getValue(), std::move(entry));
    }

    if (prevCnt) { // > 1 threads
        delete pSymbol;
        delete pEntryGroup;
        delete pBookType;
        delete pPrice;
        delete pSize;
        delete pSimulationDate;
        delete pSimulationTime;
        delete pDestination;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Receive order book updates.
 *
 * @section OPTIMIZATION FOR THREAD-SAFE BASED ON
 * LOCK-FREE TECHNOLOGY
 *
 * Since we are using various FIX fields,
 * which are all class objects, as "containers" to
 * get value (and then pass them somewhere else),
 * there is NO need to repetitively initialize
 * (i.e. call default constructors) those fields
 * WHEN onMessage is executing sequentially.
 * Also, onMessage is being called very frequently,
 * hence it will accumulate significant initialization
 * time if we define each field locally in sequential
 * onMessage.
 * To eliminate this waste of time and resources,
 * here we define fields as local static, which are
 * initialized only once and shared between multithreaded
 * onMessage instances, if any, and creates and
 * initializes their own (local) fields only if there are
 * multiple onMessage threads running at the same time.
 */
void FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.getField(numOfEntries);
    if (numOfEntries.getValue() < 1) {
        cout << "Cannot find the Entries group in MarketDataIncrementalRefresh!" << endl;
        return;
    }

    static FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    static FIX::MDEntryType bookType;
    static FIX::Symbol symbol;
    static FIX::MDEntryPx price;
    static FIX::MDEntrySize size;
    static FIX::MDEntryDate simulationDate;
    static FIX::MDEntryTime simulationTime;
    static FIX::MDMkt destination;

    // #pragma GCC diagnostic ignored ....

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries* pEntryGroup;
    FIX::MDEntryType* pBookType;
    FIX::Symbol* pSymbol;
    FIX::MDEntryPx* pPrice;
    FIX::MDEntrySize* pSize;
    FIX::MDEntryDate* pSimulationDate;
    FIX::MDEntryTime* pSimulationTime;
    FIX::MDMkt* pDestination;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pEntryGroup = &entryGroup;
        pBookType = &bookType;
        pSymbol = &symbol;
        pPrice = &price;
        pSize = &size;
        pSimulationDate = &simulationDate;
        pSimulationTime = &simulationTime;
        pDestination = &destination;
    } else { // > 1 threads; always safe way:
        pEntryGroup = new decltype(entryGroup);
        pBookType = new decltype(bookType);
        pSymbol = new decltype(symbol);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pSimulationDate = new decltype(simulationDate);
        pSimulationTime = new decltype(simulationTime);
        pDestination = new decltype(destination);
    }

    message.getGroup(1, *pEntryGroup);
    pEntryGroup->getField(*pBookType);
    pEntryGroup->getField(*pSymbol);
    pEntryGroup->getField(*pPrice);
    pEntryGroup->getField(*pSize);
    pEntryGroup->getField(*pSimulationDate);
    pEntryGroup->getField(*pSimulationTime);
    pEntryGroup->getField(*pDestination);

    OrderBookEntry entry{
        static_cast<OrderBookEntry::Type>(pBookType->getValue()),
        pSymbol->getValue(),
        pPrice->getValue(),
        static_cast<int>(pSize->getValue()),
        pDestination->getValue(),
        pSimulationDate->getValue(),
        pSimulationTime->getValue()
    };

    BCDocuments::getInstance()->onNewOBUpdateForOrderBook(pSymbol->getValue(), std::move(entry));

    if (prevCnt) { // > 1 threads
        delete pEntryGroup;
        delete pBookType;
        delete pSymbol;
        delete pPrice;
        delete pSize;
        delete pSimulationDate;
        delete pSimulationTime;
        delete pDestination;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Deal with incoming messages which type is Execution Report.
 */
void FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID&) // override
{
    FIX::ExecType execType;
    message.getField(execType);

    if (execType == FIX::ExecType_ORDER_STATUS) { // confirmation report
        FIX::NoPartyIDs numOfGroups;
        message.getField(numOfGroups);
        if (numOfGroups.getValue() < 1) {
            cout << "Cannot find any ClientID in ExecutionReport!" << endl;
            return;
        }

        static FIX::OrderID orderID;
        static FIX::OrdStatus status;
        static FIX::Symbol symbol;
        static FIX::Side orderType;
        static FIX::Price price;
        static FIX::EffectiveTime confirmTime;
        static FIX::LastMkt destination;
        static FIX::LeavesQty currentSize;
        static FIX::TransactTime serverTime;

        static FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        static FIX::PartyID userID;

        // #pragma GCC diagnostic ignored ....

        FIX::OrderID* pOrderID;
        FIX::OrdStatus* pStatus;
        FIX::Symbol* pSymbol;
        FIX::Side* pOrderType;
        FIX::Price* pPrice;
        FIX::EffectiveTime* pConfirmTime;
        FIX::LastMkt* pDestination;
        FIX::LeavesQty* pCurrentSize;
        FIX::TransactTime* pServerTime;

        FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup;
        FIX::PartyID* pUserID;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pOrderID = &orderID;
            pStatus = &status;
            pSymbol = &symbol;
            pOrderType = &orderType;
            pPrice = &price;
            pConfirmTime = &confirmTime;
            pDestination = &destination;
            pCurrentSize = &currentSize;
            pServerTime = &serverTime;
            pIDGroup = &idGroup;
            pUserID = &userID;
        } else { // > 1 threads; always safe way:
            pOrderID = new decltype(orderID);
            pStatus = new decltype(status);
            pSymbol = new decltype(symbol);
            pOrderType = new decltype(orderType);
            pPrice = new decltype(price);
            pConfirmTime = new decltype(confirmTime);
            pDestination = new decltype(destination);
            pCurrentSize = new decltype(currentSize);
            pServerTime = new decltype(serverTime);
            pIDGroup = new decltype(idGroup);
            pUserID = new decltype(userID);
        }

        message.getField(*pOrderID);
        message.getField(*pStatus);
        message.getField(*pSymbol);
        message.getField(*pOrderType);
        message.getField(*pPrice);
        message.getField(*pConfirmTime);
        message.getField(*pDestination);
        message.getField(*pCurrentSize);
        message.getField(*pServerTime);

        message.getGroup(1, *pIDGroup);
        pIDGroup->getField(*pUserID);

        cout << "Confirmation Report: "
             << pUserID->getValue() << "\t"
             << pOrderID->getValue() << "\t"
             << pOrderType->getValue() << "\t"
             << pSymbol->getValue() << "\t"
             << pCurrentSize->getValue() << "\t"
             << pPrice->getValue() << "\t"
             << pStatus->getValue() << "\t"
             << pDestination->getValue() << "\t"
             << pConfirmTime->getString() << "\t"
             << pServerTime->getString() << endl;

        ExecutionReport report{
            pUserID->getValue(),
            pOrderID->getValue(),
            static_cast<Order::Type>(pOrderType->getValue()),
            pSymbol->getValue(),
            static_cast<int>(pCurrentSize->getValue()),
            0, // executed size
            pPrice->getValue(),
            static_cast<Order::Status>(pStatus->getValue()),
            pDestination->getValue(),
            pConfirmTime->getValue(),
            pServerTime->getValue()
        };

        BCDocuments::getInstance()->onNewExecutionReportForUserRiskManagement(pUserID->getValue(), std::move(report));

        if (prevCnt) { // > 1 threads
            delete pOrderID;
            delete pStatus;
            delete pSymbol;
            delete pOrderType;
            delete pPrice;
            delete pConfirmTime;
            delete pDestination;
            delete pCurrentSize;
            delete pServerTime;
            delete pIDGroup;
            delete pUserID;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);

    } else { // FIX::ExecType_TRADE: execution report
        FIX::NoPartyIDs numOfGroups;
        message.getField(numOfGroups);
        if (numOfGroups.getValue() < 2) {
            cout << "Cannot find all ClientID in ExecutionReport!" << endl;
            return;
        }

        static FIX::OrderID orderID1;
        static FIX::SecondaryOrderID orderID2;
        static FIX::OrdStatus status;
        static FIX::Symbol symbol;
        static FIX::Side orderType1;
        static FIX::OrdType orderType2;
        static FIX::Price price;
        static FIX::EffectiveTime execTime;
        static FIX::LastMkt destination;
        static FIX::CumQty executedSize;
        static FIX::TransactTime serverTime;

        static FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        static FIX::PartyID userID1;
        static FIX::PartyID userID2;

        static FIX50SP2::ExecutionReport::NoTrdRegTimestamps timeGroup;
        static FIX::TrdRegTimestamp utcTime1;
        static FIX::TrdRegTimestamp utcTime2;

        // #pragma GCC diagnostic ignored ....

        FIX::OrderID* pOrderID1;
        FIX::SecondaryOrderID* pOrderID2;
        FIX::OrdStatus* pStatus;
        FIX::Symbol* pSymbol;
        FIX::Side* pOrderType1;
        FIX::OrdType* pOrderType2;
        FIX::Price* pPrice;
        FIX::EffectiveTime* pExecTime;
        FIX::LastMkt* pDestination;
        FIX::CumQty* pExecutedSize;
        FIX::TransactTime* pServerTime;

        FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup;
        FIX::PartyID* pUserID1;
        FIX::PartyID* pUserID2;

        FIX50SP2::ExecutionReport::NoTrdRegTimestamps* pTimeGroup;
        FIX::TrdRegTimestamp* pUTCTime1;
        FIX::TrdRegTimestamp* pUTCTime2;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = s_cntAtom.load(std::memory_order_relaxed);

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) { // sequential case; optimized:
            pOrderID1 = &orderID1;
            pOrderID2 = &orderID2;
            pStatus = &status;
            pSymbol = &symbol;
            pOrderType1 = &orderType1;
            pOrderType2 = &orderType2;
            pPrice = &price;
            pExecTime = &execTime;
            pDestination = &destination;
            pExecutedSize = &executedSize;
            pServerTime = &serverTime;
            pIDGroup = &idGroup;
            pUserID1 = &userID1;
            pUserID2 = &userID2;
            pTimeGroup = &timeGroup;
            pUTCTime1 = &utcTime1;
            pUTCTime2 = &utcTime2;
        } else { // > 1 threads; always safe way:
            pOrderID1 = new decltype(orderID1);
            pOrderID2 = new decltype(orderID2);
            pStatus = new decltype(status);
            pSymbol = new decltype(symbol);
            pOrderType1 = new decltype(orderType1);
            pOrderType2 = new decltype(orderType2);
            pPrice = new decltype(price);
            pExecTime = new decltype(execTime);
            pDestination = new decltype(destination);
            pExecutedSize = new decltype(executedSize);
            pServerTime = new decltype(serverTime);
            pIDGroup = new decltype(idGroup);
            pUserID1 = new decltype(userID1);
            pUserID2 = new decltype(userID2);
            pTimeGroup = new decltype(timeGroup);
            pUTCTime1 = new decltype(utcTime1);
            pUTCTime2 = new decltype(utcTime2);
        }

        message.getField(*pOrderID1);
        message.getField(*pOrderID2);
        message.getField(*pStatus);
        message.getField(*pSymbol);
        message.getField(*pOrderType1);
        message.getField(*pOrderType2);
        message.getField(*pPrice);
        message.getField(*pExecTime);
        message.getField(*pDestination);
        message.getField(*pExecutedSize);
        message.getField(*pServerTime);

        message.getGroup(1, *pIDGroup);
        pIDGroup->getField(*pUserID1);
        message.getGroup(2, *pIDGroup);
        pIDGroup->getField(*pUserID2);

        message.getGroup(1, *pTimeGroup);
        pTimeGroup->getField(*pUTCTime1);
        message.getGroup(2, *pTimeGroup);
        pTimeGroup->getField(*pUTCTime2);

        auto printRpts = [](bool rpt1or2, auto userID, auto orderID, auto orderType, auto symbol, auto executedSize, auto price, auto status, auto destination, auto execTime, auto serverTime) {
            cout << (rpt1or2 ? "Report1: " : "Report2: ")
                 << userID->getValue() << "\t"
                 << orderID->getValue() << "\t"
                 << orderType->getValue() << "\t"
                 << symbol->getValue() << "\t"
                 << executedSize->getValue() << "\t"
                 << price->getValue() << "\t"
                 << status->getValue() << "\t"
                 << destination->getValue() << "\t"
                 << execTime->getString() << "\t"
                 << serverTime->getString() << endl;
        };

        if (pStatus->getValue() != FIX::OrdStatus_REPLACED) { // == '5', means this is a trade update from TRTH -> no need to store it
            TradingRecord record{
                pServerTime->getValue(),
                pExecTime->getValue(),
                pSymbol->getValue(),
                pPrice->getValue(),
                static_cast<int>(pExecutedSize->getValue()),
                pUserID1->getValue(),
                pUserID2->getValue(),
                pOrderID1->getValue(),
                pOrderID2->getValue(),
                static_cast<Order::Type>(pOrderType1->getValue()),
                static_cast<Order::Type>(pOrderType2->getValue()),
                pStatus->getValue(), // decision
                pDestination->getValue(),
                pUTCTime1->getValue(),
                pUTCTime2->getValue()
            };

            DBConnector::getInstance()->insertTradingRecord(record);
        }

        switch (pStatus->getValue()) {
        case FIX::OrdStatus_FILLED:
        case FIX::OrdStatus_REPLACED: {
            Transaction transac = {
                pSymbol->getValue(),
                static_cast<int>(pExecutedSize->getValue()),
                pPrice->getValue(),
                pDestination->getValue(),
                pExecTime->getValue()
            };

            FIXAcceptor::getInstance()->sendLastPrice2All(transac);

            if (FIX::OrdStatus_FILLED == pStatus->getValue()) { // trade
                printRpts(true, pUserID1, pOrderID1, pOrderType1, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);
                printRpts(false, pUserID2, pOrderID2, pOrderType2, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);

                ExecutionReport report1{
                    pUserID1->getValue(),
                    pOrderID1->getValue(),
                    static_cast<Order::Type>(pOrderType1->getValue()),
                    pSymbol->getValue(),
                    0, // current size: will be added later
                    static_cast<int>(pExecutedSize->getValue()),
                    pPrice->getValue(),
                    static_cast<Order::Status>(pStatus->getValue()),
                    pDestination->getValue(),
                    pExecTime->getValue(),
                    pServerTime->getValue()
                };

                ExecutionReport report2{
                    pUserID2->getValue(),
                    pOrderID2->getValue(),
                    static_cast<Order::Type>(pOrderType2->getValue()),
                    pSymbol->getValue(),
                    0, // current size: will be added later
                    static_cast<int>(pExecutedSize->getValue()),
                    pPrice->getValue(),
                    static_cast<Order::Status>(pStatus->getValue()),
                    pDestination->getValue(),
                    pExecTime->getValue(),
                    pServerTime->getValue()
                };

                auto* docs = BCDocuments::getInstance();
                docs->onNewTransacForCandlestickData(pSymbol->getValue(), transac);
                docs->onNewExecutionReportForUserRiskManagement(pUserID1->getValue(), std::move(report1));
                docs->onNewExecutionReportForUserRiskManagement(pUserID2->getValue(), std::move(report2));
            } else { // FIX::OrdStatus_REPLACED: TRTH trade
                BCDocuments::getInstance()->onNewTransacForCandlestickData(pSymbol->getValue(), transac);
            }
        } break;
        case FIX::OrdStatus_CANCELED: { // cancellation
            printRpts(true, pUserID1, pOrderID1, pOrderType1, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);
            printRpts(false, pUserID2, pOrderID2, pOrderType2, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);

            ExecutionReport report2{
                pUserID2->getValue(),
                pOrderID2->getValue(),
                static_cast<Order::Type>(pOrderType2->getValue()),
                pSymbol->getValue(),
                0, // current size: will be added later
                static_cast<int>(pExecutedSize->getValue()),
                pPrice->getValue(),
                static_cast<Order::Status>(pStatus->getValue()),
                pDestination->getValue(),
                pExecTime->getValue(),
                pServerTime->getValue()
            };

            BCDocuments::getInstance()->onNewExecutionReportForUserRiskManagement(pUserID2->getValue(), std::move(report2));
        } break;
        } // switch

        if (prevCnt) { // > 1 threads
            delete pOrderID1;
            delete pOrderID2;
            delete pStatus;
            delete pSymbol;
            delete pOrderType1;
            delete pOrderType2;
            delete pPrice;
            delete pExecTime;
            delete pDestination;
            delete pExecutedSize;
            delete pServerTime;
            delete pIDGroup;
            delete pUserID1;
            delete pUserID2;
            delete pUTCTime2;
            delete pUTCTime1;
            delete pTimeGroup;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);
    }
}
