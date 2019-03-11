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

// Predefined constant FIX fields (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1"); // FIX BeginString
static const auto& FIXFIELD_CLIENTID = FIX::PartyRole(3); // 3 = ClientID in FIX4.2
static const auto& FIXFIELD_EXECTYPE_NEW = FIX::ExecType('0'); // '0' = New

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
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(order.getOrderID()));
    message.setField(FIX::Symbol(order.getSymbol()));
    message.setField(FIX::Side(order.getOrderType())); // FIXME: separate Side and OrdType
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(order.getShareSize()));
    message.setField(FIX::OrdType(order.getOrderType())); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(order.getPrice()));

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_CLIENTID);
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
    s_senderID = sessionID.getSenderCompID();
    s_targetID = sessionID.getTargetCompID();
    // cout << "SenderID: " << s_senderID << "\n"
    //      << "TargetID: " << s_targetID << endl;
}

void FIXInitiator::onLogon(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logon - " << sessionID << endl;
}

void FIXInitiator::onLogout(const FIX::SessionID& sessionID) // override
{
    cout << endl
         << "Logout - " << sessionID << endl;
}

void FIXInitiator::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID); // Message is message type
}

/**
 * @brief Deal with the incoming message
 * which type is excution report
 */
void FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID&) // override
{
    FIX::ExecType execType;
    FIX::OrdStatus orderStatus;
    message.get(execType);
    message.get(orderStatus); // actions.decision

    if (execType == ::FIXFIELD_EXECTYPE_NEW) {
        // Update Version: ClientID has been replaced by PartyRole and PartyID
        FIX::NoPartyIDs numOfGroup;
        message.get(numOfGroup);
        if (numOfGroup < 1) {
            cout << "Cannot find any ClientID in ExecutionReport !" << endl;
            return;
        }

        // For confirmation report // sendOrderConfirms
        FIX::OrderID orderID;
        FIX::Symbol symbol;
        FIX::Side orderType;
        FIX::Price price;
        FIX::EffectiveTime confirmTime;
        FIX::LeavesQty shareSize;
        FIX::TransactTime serverTime;

        message.get(orderID);
        message.get(symbol);
        message.get(orderType);
        message.get(price);
        message.get(confirmTime);
        message.get(shareSize);
        message.get(serverTime);

        FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        FIX::PartyRole role;
        FIX::PartyID username;
        for (int i = 1; i <= numOfGroup; i++) {
            message.getGroup(i, idGroup);
            idGroup.get(role);
            if (role == 3) { // 3 -> formerly FIX 4.2 ClientID
                idGroup.get(username);
            }
        }

        Report report{
            username,
            orderID,
            orderStatus,
            symbol,
            orderType,
            price,
            shareSize,
            serverTime.getValue(),
            confirmTime.getValue()
        };

        cout << "ConfirmRepo: "
             << username << "\t"
             << orderID << "\t"
             << orderStatus << "\t"
             << serverTime.getString() << "\t"
             << confirmTime.getString() << "\t"
             << symbol << "\t"
             << orderType << "\t"
             << price << "\t"
             << shareSize << endl;
        BCDocuments::getInstance()->onNewReportForUserRiskManagement(username, report);

    } else { // For execution report
        static FIX::OrderID orderID1;
        static FIX::SecondaryOrderID orderID2;
        static FIX::Symbol symbol;
        static FIX::Side orderType1;
        static FIX::OrdType orderType2;
        static FIX::Price price;
        static FIX::EffectiveTime execTime;
        static FIX::LastMkt destination;
        static FIX::LeavesQty leftQty;
        static FIX::CumQty shareSize;
        static FIX::TransactTime serverTime;

        static FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        static FIX::PartyID username1;
        static FIX::PartyID username2;

        // #pragma GCC diagnostic ignored ....

        FIX::Symbol* pSymbol;
        FIX::OrderID* pOrderID1;
        FIX::SecondaryOrderID* pOrderID2;
        FIX::Side* pOrderType1;
        FIX::OrdType* pOrderType2;
        FIX::Price* pPrice;
        FIX::EffectiveTime* pExecTime;
        FIX::LastMkt* pDestination;
        FIX::LeavesQty* pLeftQty;
        FIX::CumQty* pShareSize;
        FIX::TransactTime* pServerTime;

        FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup;
        FIX::PartyID* pUsername1;
        FIX::PartyID* pUsername2;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            continue;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) {
            pSymbol = &symbol;
            pOrderID1 = &orderID1;
            pOrderType1 = &orderType1;
            pOrderID2 = &orderID2;
            pOrderType2 = &orderType2;
            pPrice = &price;
            pDestination = &destination;
            pExecTime = &execTime;
            pLeftQty = &leftQty;
            pShareSize = &shareSize;
            pServerTime = &serverTime;
            pIDGroup = &idGroup;
            pUsername1 = &username1;
            pUsername2 = &username2;
        } else {
            // note: important precondition of the correctness of the following code is: std::operator new() is thread-safe.
            pSymbol = new decltype(symbol);
            pOrderID1 = new decltype(orderID1);
            pOrderType1 = new decltype(orderType1);
            pOrderID2 = new decltype(orderID2);
            pOrderType2 = new decltype(orderType2);
            pPrice = new decltype(price);
            pDestination = new decltype(destination);
            pExecTime = new decltype(execTime);
            pLeftQty = new decltype(leftQty);
            pShareSize = new decltype(shareSize);
            pServerTime = new decltype(serverTime);
            pIDGroup = new decltype(idGroup);
            pUsername1 = new decltype(username1);
            pUsername2 = new decltype(username2);
        }

        message.get(*pSymbol);
        message.get(*pOrderID1);
        message.get(*pOrderType1);
        message.get(*pOrderID2);
        message.get(*pOrderType2);
        message.get(*pPrice);
        message.get(*pDestination);
        message.get(*pExecTime);
        message.get(*pLeftQty);
        message.get(*pShareSize);
        message.get(*pServerTime);

        // Update Version: ClientID has been replaced by PartyRole and PartyID
        FIX::NoPartyIDs numOfGroup;
        message.get(numOfGroup);
        if (numOfGroup < 2) {
            cout << "Cannot find all ClientID in ExecutionReport !" << endl;
            return;
        }

        message.getGroup(1, *pIDGroup);
        pIDGroup->get(*pUsername1);
        message.getGroup(2, *pIDGroup);
        pIDGroup->get(*pUsername2);

        auto printRpts = [](bool rpt1or2, auto pUsername, auto pOrderID, auto orderStatus, auto pServerTime, auto pExecTime, auto pSymbol, auto pOrderType, auto pPrice, auto pShareSize) {
            cout << (rpt1or2 ? "Report1: " : "Report2: ")
                 << *pUsername << "\t"
                 << *pOrderID << "\t"
                 << orderStatus << "\t"
                 << pServerTime->getString() << "\t"
                 << pExecTime->getString() << "\t"
                 << *pSymbol << "\t"
                 << *pOrderType << "\t"
                 << *pPrice << "\t"
                 << *pShareSize << endl;
        };

        switch (orderStatus) {
        case FIX::ExecType_FILL:
        case FIX::ExecType_CANCELED: {
            Transaction transac = {
                *pSymbol,
                *pPrice,
                int(*pShareSize),
                *pDestination,
                pExecTime->getValue()
            };

            if (FIX::ExecType_FILL == orderStatus) { // TRADE
                Report report1{
                    *pUsername1,
                    *pOrderID1,
                    orderStatus,
                    *pSymbol,
                    *pOrderType1,
                    *pPrice,
                    *pShareSize,
                    pServerTime->getValue(),
                    pExecTime->getValue()
                };

                Report report2{
                    *pUsername2,
                    *pOrderID2,
                    orderStatus,
                    *pSymbol,
                    *pOrderType2,
                    *pPrice,
                    *pShareSize,
                    pServerTime->getValue(),
                    pExecTime->getValue()
                };

                printRpts(true, pUsername1, pOrderID1, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType1, pPrice, pShareSize);
                printRpts(false, pUsername2, pOrderID2, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType2, pPrice, pShareSize);

                auto* docs = BCDocuments::getInstance();
                docs->onNewTransacForCandlestickData(*pSymbol, transac);
                docs->onNewReportForUserRiskManagement(*pUsername1, report1);
                docs->onNewReportForUserRiskManagement(*pUsername2, report2);
            } else { // TRTH TRADE
                BCDocuments::getInstance()->onNewTransacForCandlestickData(*pSymbol, transac);
            }

            FIXAcceptor::getInstance()->sendLastPrice2All(transac);
        } break;
        case FIX::ExecType_EXPIRED: { // CANCEL
            Report report2{
                *pUsername2,
                *pOrderID2,
                orderStatus,
                *pSymbol,
                *pOrderType2,
                *pPrice,
                *pShareSize,
                pServerTime->getValue(),
                pExecTime->getValue()
            };

            BCDocuments::getInstance()->onNewReportForUserRiskManagement(*pUsername2, report2);
            printRpts(true, pUsername1, pOrderID1, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType1, pPrice, pShareSize);
            printRpts(false, pUsername2, pOrderID2, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType2, pPrice, pShareSize);
        } break;
        } // switch

        if (prevCnt) { // > 1 threads
            delete pSymbol;
            delete pOrderID1;
            delete pOrderID2;
            delete pUsername1;
            delete pUsername2;
            delete pOrderType1;
            delete pOrderType2;
            delete pShareSize;
            delete pPrice;
            delete pLeftQty;
            delete pServerTime;
            delete pExecTime;
            delete pDestination;
        }

        s_cntAtom--;
        assert(s_cntAtom >= 0);
    }
}

/**
 * @brief receive order book updates
 *
 * @section OPTIMIZATION FOR THREAD-SAFE BASED ON
 * LOCK-FREE TECHNOLOGY
 *
 * Since we are using various FIX fields,
 * which are all class objects, as "containers" to
 * get value(and then pass them somewhere else),
 * there is NO need to repetitively initialize
 * (i.e. call default constructors) those fields
 * WHEN onMessage is executing sequentially.
 * Also, onMessage is being called very frequently,
 * hence it will accumulate significant initialization
 * time if we define each field locally in sequential
 * onMessage.
 * To eliminate that wastes, here we define fields
 * as local static, which are initialized only once
 * and shared between multithreaded onMessage instances,
 * if any, and creates and initializes their own(local)
 * fields only if there are multiple onMessage threads
 * are running at the time.
 */
void FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntry;
    message.get(numOfEntry);
    if (numOfEntry < 1) {
        cout << "Cannot find the entry group in messge !" << endl;
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
        pDestination = &destination;
        pDate = &date;
        pDaytime = &daytime;
    } else { // > 1 threads; always safe way:
        pEntryGroup = new decltype(entryGroup);
        pBookType = new decltype(bookType);
        pSymbol = new decltype(symbol);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pDestination = new decltype(destination);
        pDate = new decltype(date);
        pDaytime = new decltype(daytime);
    }

    message.getGroup(1, *pEntryGroup);
    pEntryGroup->get(*pBookType);
    pEntryGroup->get(*pSymbol);
    pEntryGroup->get(*pPrice);
    pEntryGroup->get(*pSize);
    pEntryGroup->get(*pDate);
    pEntryGroup->get(*pDaytime);
    pEntryGroup->get(*pDestination);

    OrderBookEntry update{
        OrderBookEntry::s_toOrderBookType(*pBookType),
        pSymbol->getString(),
        double{ *pPrice },
        double{ *pSize },
        pDestination->getString(),
        pDate->getValue(),
        pDaytime->getValue()
    };

    BCDocuments::getInstance()->onNewOBEntryForOrderBook(*pSymbol, update);

    if (prevCnt) { // > 1 threads
        delete pEntryGroup;
        delete pBookType;
        delete pSymbol;
        delete pPrice;
        delete pSize;
        delete pDestination;
        delete pDate;
        delete pDaytime;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
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

    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find Symbol in SecurityList from ME" << endl;
    }

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    FIX::Symbol symbol;

    auto* docs = BCDocuments::getInstance();
    for (int i = 1; i <= numOfGroup; i++) {
        message.getGroup(i, relatedSymGroup);
        relatedSymGroup.get(symbol);

        docs->addSymbol(symbol);
        docs->attachOrderBookToSymbol(symbol);
        docs->attachCandlestickDataToSymbol(symbol);
    }

    // Now, it's safe to advance all routines that *read* permanent data structures created above:
    BCDocuments::s_isSecurityListReady = true;
}
