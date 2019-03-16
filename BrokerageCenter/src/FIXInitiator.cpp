/*
** Connector to MatchingEngine
**/

#include "FIXInitiator.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"

#include <atomic>
#include <cassert>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

/* static */ std::string FIXInitiator::s_senderID;
/* static */ std::string FIXInitiator::s_targetID;

// Predefined constant FIX message fields (to avoid recalculations):
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

    if (commonDict.has("FileLogPath")) {
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
    } else {
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, verbose));
    }

    if (commonDict.has("FileStorePath")) {
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
    } else {
        m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    }

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
 * @brief Sending the order to the server
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

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_PARTYROLE_CLIENTID);
    idGroup.set(FIX::PartyID(order.getUsername()));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Method called when a new Session was created.
 * Set Sender and Target Comp ID.
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
    crack(message, sessionID); // Message is message type
}

/*
 * @brief Receive security list from ME. Only run once each system session.
 */
void FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID&) // override
{
    // This test is required because if there is a disconnection between ME and BC,
    // the ME will send the security list again during the reconnection procedure.
    if (BCDocuments::s_isSecurityListReady)
        return;

    FIX::NoRelatedSym numOfGroups;
    message.get(numOfGroups);
    if (numOfGroups < 1) {
        cout << "Cannot find any Symbol in SecurityList!" << endl;
        return;
    }

    static FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    static FIX::Symbol symbol;

    // #pragma GCC diagnostic ignored ....

    FIX50SP2::SecurityList::NoRelatedSym* pRelatedSymGroup;
    FIX::Symbol* pSymbol;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pRelatedSymGroup = &relatedSymGroup;
        pSymbol = &symbol;
    } else { // > 1 threads; always safe way:
        pRelatedSymGroup = new decltype(relatedSymGroup);
        pSymbol = new decltype(symbol);
    }

    auto* docs = BCDocuments::getInstance();
    for (int i = 1; i <= numOfGroups.getValue(); i++) {
        message.getGroup(static_cast<unsigned int>(i), *pRelatedSymGroup);
        pRelatedSymGroup->get(*pSymbol);

        docs->addSymbol(pSymbol->getValue());
        docs->attachOrderBookToSymbol(pSymbol->getValue());
        docs->attachCandlestickDataToSymbol(pSymbol->getValue());
    }

    // Now, it's safe to advance all routines that *read* permanent data structures created above:
    BCDocuments::s_isSecurityListReady = true;

    if (prevCnt) { // > 1 threads
        delete pRelatedSymGroup;
        delete pSymbol;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Receive order book updates
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
    message.get(numOfEntries);
    if (numOfEntries < 1) {
        cout << "Cannot find the Entries group in MarketDataIncrementalRefresh!" << endl;
        return;
    }

    static FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    static FIX::MDEntryType bookType;
    static FIX::Symbol symbol;
    static FIX::MDEntryPx price;
    static FIX::MDEntrySize size;
    static FIX::MDEntryDate date;
    static FIX::MDEntryTime daytime;
    static FIX::MDMkt destination;

    // #pragma GCC diagnostic ignored ....

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries* pEntryGroup;
    FIX::MDEntryType* pBookType;
    FIX::Symbol* pSymbol;
    FIX::MDEntryPx* pPrice;
    FIX::MDEntrySize* pSize;
    FIX::MDEntryDate* pDate;
    FIX::MDEntryTime* pDaytime;
    FIX::MDMkt* pDestination;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        continue;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pEntryGroup = &entryGroup;
        pBookType = &bookType;
        pSymbol = &symbol;
        pPrice = &price;
        pSize = &size;
        pDate = &date;
        pDaytime = &daytime;
        pDestination = &destination;
    } else { // > 1 threads; always safe way:
        pEntryGroup = new decltype(entryGroup);
        pBookType = new decltype(bookType);
        pSymbol = new decltype(symbol);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pDate = new decltype(date);
        pDaytime = new decltype(daytime);
        pDestination = new decltype(destination);
    }

    message.getGroup(1, *pEntryGroup);
    pEntryGroup->get(*pBookType);
    pEntryGroup->get(*pSymbol);
    pEntryGroup->get(*pPrice);
    pEntryGroup->get(*pSize);
    pEntryGroup->get(*pDate);
    pEntryGroup->get(*pDaytime);
    pEntryGroup->get(*pDestination);

    OrderBookEntry entry{
        static_cast<OrderBookEntry::Type>(pBookType->getValue()),
        pSymbol->getValue(),
        pPrice->getValue(),
        static_cast<int>(pSize->getValue()),
        pDestination->getValue(),
        pDate->getValue(),
        pDaytime->getValue()
    };

    BCDocuments::getInstance()->onNewOBEntryForOrderBook(pSymbol->getValue(), std::move(entry));

    if (prevCnt) { // > 1 threads
        delete pEntryGroup;
        delete pBookType;
        delete pSymbol;
        delete pPrice;
        delete pSize;
        delete pDate;
        delete pDaytime;
        delete pDestination;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/**
 * @brief Deal with incoming messages which type is Execution Report
 */
void FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID&) // override
{
    FIX::ExecType execType;
    message.get(execType);

    if (execType == FIX::ExecType_ORDER_STATUS) { // Confirmation Report
        FIX::NoPartyIDs numOfGroups;
        message.get(numOfGroups);
        if (numOfGroups < 1) {
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
        static FIX::PartyID username;

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
        FIX::PartyID* pUsername;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

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
            pUsername = &username;
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
            pUsername = new decltype(username);
        }

        message.get(*pOrderID);
        message.get(*pStatus);
        message.get(*pSymbol);
        message.get(*pOrderType);
        message.get(*pPrice);
        message.get(*pConfirmTime);
        message.get(*pDestination);
        message.get(*pCurrentSize);
        message.get(*pServerTime);

        message.getGroup(1, *pIDGroup);
        pIDGroup->get(*pUsername);

        cout << "Confirmation Report: "
             << pUsername->getValue() << "\t"
             << pOrderID->getValue() << "\t"
             << pOrderType->getValue() << "\t"
             << pSymbol->getValue() << "\t"
             << pCurrentSize->getValue() << "\t"
             << pPrice->getValue() << "\t"
             << pStatus->getValue() << "\t"
             << pDestination->getValue() << "\t"
             << pConfirmTime->getString() << "\t"
             << pServerTime->getString() << endl;

        Report report{
            pUsername->getValue(),
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

        BCDocuments::getInstance()->onNewReportForUserRiskManagement(username, std::move(report));

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
            delete pUsername;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);

    } else { // FIX::ExecType_TRADE: Execution Report
        FIX::NoPartyIDs numOfGroups;
        message.get(numOfGroups);
        if (numOfGroups < 2) {
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
        static FIX::PartyID username1;
        static FIX::PartyID username2;

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
        FIX::PartyID* pUsername1;
        FIX::PartyID* pUsername2;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

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
            pUsername1 = &username1;
            pUsername2 = &username2;
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
            pUsername1 = new decltype(username1);
            pUsername2 = new decltype(username2);
        }

        message.get(*pOrderID1);
        message.get(*pOrderID2);
        message.get(*pStatus);
        message.get(*pSymbol);
        message.get(*pOrderType1);
        message.get(*pOrderType2);
        message.get(*pPrice);
        message.get(*pExecTime);
        message.get(*pDestination);
        message.get(*pExecutedSize);
        message.get(*pServerTime);

        message.getGroup(1, *pIDGroup);
        pIDGroup->get(*pUsername1);
        message.getGroup(2, *pIDGroup);
        pIDGroup->get(*pUsername2);

        auto printRpts = [](bool rpt1or2, auto username, auto orderID, auto orderType, auto symbol, auto executedSize, auto price, auto status, auto destination, auto execTime, auto serverTime) {
            cout << (rpt1or2 ? "Report1: " : "Report2: ")
                 << username->getValue() << "\t"
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

        switch (*pStatus) {
        case FIX::OrdStatus_FILLED:
        case FIX::OrdStatus_REPLACED: {
            Transaction transac = {
                pSymbol->getValue(),
                static_cast<int>(pExecutedSize->getValue()),
                pPrice->getValue(),
                pDestination->getValue(),
                pExecTime->getValue()
            };

            if (FIX::OrdStatus_FILLED == *pStatus) { // TRADE
                printRpts(true, pUsername1, pOrderID1, pOrderType1, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);
                printRpts(false, pUsername2, pOrderID2, pOrderType2, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);

                Report report1{
                    pUsername1->getValue(),
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

                Report report2{
                    pUsername2->getValue(),
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
                docs->onNewReportForUserRiskManagement(pUsername1->getValue(), std::move(report1));
                docs->onNewReportForUserRiskManagement(pUsername2->getValue(), std::move(report2));
            } else { // FIX::OrdStatus_REPLACED: TRTH TRADE
                BCDocuments::getInstance()->onNewTransacForCandlestickData(pSymbol->getValue(), transac);
            }

            FIXAcceptor::getInstance()->sendLastPrice2All(transac);
        } break;
        case FIX::OrdStatus_CANCELED: { // CANCELLATION
            printRpts(true, pUsername1, pOrderID1, pOrderType1, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);
            printRpts(false, pUsername2, pOrderID2, pOrderType2, pSymbol, pExecutedSize, pPrice, pStatus, pDestination, pExecTime, pServerTime);

            Report report2{
                pUsername2->getValue(),
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

            BCDocuments::getInstance()->onNewReportForUserRiskManagement(username2.getValue(), std::move(report2));
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
            delete pUsername1;
            delete pUsername2;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);
    }
}
