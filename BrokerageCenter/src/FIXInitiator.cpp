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
void FIXInitiator::sendQuote(const Quote& quote)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(s_targetID));
    header.setField(FIX::MsgType(FIX::MsgType_NewOrderSingle));

    message.setField(FIX::ClOrdID(quote.getOrderID()));
    message.setField(FIX::Symbol(quote.getSymbol()));
    message.setField(FIX::Side(quote.getOrderType())); // FIXME: separate Side and OrdType
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::OrderQty(quote.getShareSize()));
    message.setField(FIX::OrdType(quote.getOrderType())); // FIXME: separate Side and OrdType
    message.setField(FIX::Price(quote.getPrice()));

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_CLIENTID);
    idGroup.set(FIX::PartyID(quote.getUserName()));
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

void FIXInitiator::toApp(FIX::Message& message, const FIX::SessionID&) throw(FIX::DoNotSend) // override
{
    try {
        FIX::PossDupFlag possDupFlag;
        message.getHeader().getField(possDupFlag);
        if (possDupFlag)
            throw FIX::DoNotSend();
    } catch (FIX::FieldNotFound&) {
    }
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

        // For confirmation report // sendQuoteConfirms
        FIX::OrderID orderID;
        FIX::Symbol symbol;
        FIX::Side orderType;
        FIX::Price price;
        FIX::LeavesQty shareSize;
        FIX::TransactTime serverTime;
        FIX::EffectiveTime confirmTime;

        message.get(orderID);
        message.get(symbol);
        message.get(orderType);
        message.get(price);
        message.get(shareSize);
        message.get(serverTime);
        message.get(confirmTime);

        FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        FIX::PartyRole role;
        FIX::PartyID userName;
        for (int i = 1; i <= numOfGroup; i++) {
            message.getGroup(i, idGroup);
            idGroup.get(role);
            if (role == 3) { // 3 -> formerly FIX 4.2 ClientID
                idGroup.get(userName);
            }
        }

        Report report{
            userName,
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
             << userName << "\t"
             << orderID << "\t"
             << orderStatus << "\t"
             << serverTime.getString() << "\t"
             << confirmTime.getString() << "\t"
             << symbol << "\t"
             << orderType << "\t"
             << price << "\t"
             << shareSize << endl;
        BCDocuments::getInstance()->addReportToUserRiskManagement(userName, report);

    } else { // For execution report
        static FIX::OrderID orderID1;
        static FIX::SecondaryOrderID orderID2;
        static FIX::Symbol symbol;
        static FIX::Side orderType1;
        static FIX::OrdType orderType2;
        static FIX::LeavesQty leftQty;
        static FIX::CumQty shareSize;
        static FIX::Price price;
        static FIX::TransactTime serverTime;
        static FIX::EffectiveTime execTime;

        static FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        static FIX::PartyID userName1;
        static FIX::PartyID userName2;
        static FIX::PartyID destination;

        // #pragma GCC diagnostic ignored ....

        FIX::Symbol* pSymbol;
        FIX::OrderID* pOrderID1;
        FIX::SecondaryOrderID* pOrderID2;
        FIX::Side* pOrderType1;
        FIX::OrdType* pOrderType2;
        FIX::CumQty* pShareSize;
        FIX::Price* pPrice;
        FIX::LeavesQty* pLeftQty;
        FIX::TransactTime* pServerTime;
        FIX::EffectiveTime* pExecTime;

        FIX50SP2::ExecutionReport::NoPartyIDs* pIDGroup;
        FIX::PartyID* pUserName1;
        FIX::PartyID* pUserName2;
        FIX::PartyID* pDestination;

        static std::atomic<unsigned int> s_cntAtom{ 0 };
        unsigned int prevCnt = 0;

        while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
            ;
        assert(s_cntAtom > 0);

        if (0 == prevCnt) {
            pSymbol = &symbol;
            pOrderID1 = &orderID1;
            pOrderType1 = &orderType1;
            pOrderID2 = &orderID2;
            pOrderType2 = &orderType2;
            pShareSize = &shareSize;
            pPrice = &price;
            pLeftQty = &leftQty;
            pServerTime = &serverTime;
            pExecTime = &execTime;
            pIDGroup = &idGroup;
            pUserName1 = &userName1;
            pUserName2 = &userName2;
            pDestination = &destination;
        } else {
            pSymbol = new decltype(symbol);
            pOrderID1 = new decltype(orderID1);
            pOrderType1 = new decltype(orderType1);
            pOrderID2 = new decltype(orderID2);
            pOrderType2 = new decltype(orderType2);
            pShareSize = new decltype(shareSize);
            pPrice = new decltype(price);
            pLeftQty = new decltype(leftQty);
            pServerTime = new decltype(serverTime);
            pExecTime = new decltype(execTime);
            pIDGroup = new decltype(idGroup);
            pUserName1 = new decltype(userName1);
            pUserName2 = new decltype(userName2);
            pDestination = new decltype(destination);
        }

        message.get(*pSymbol);
        message.get(*pOrderID1);
        message.get(*pOrderType1);
        message.get(*pOrderID2);
        message.get(*pOrderType2);
        message.get(*pShareSize);
        message.get(*pPrice);
        message.get(*pLeftQty);
        message.get(*pServerTime);
        message.get(*pExecTime);

        // Update Version: ClientID and ExecBroker has been replaced by PartyRole and PartyID
        FIX::NoPartyIDs numOfGroup;
        message.get(numOfGroup);
        if (numOfGroup < 3) {
            cout << "Cannot find all ClientID in ExecutionReport !" << endl;
            return;
        }

        message.getGroup(1, *pIDGroup);
        pIDGroup->get(*pUserName1);
        message.getGroup(2, *pIDGroup);
        pIDGroup->get(*pUserName2);
        message.getGroup(3, *pIDGroup);
        pIDGroup->get(*pDestination);

        auto printRpts = [](bool rpt1or2, auto pUserName, auto pOrderID, auto orderStatus, auto pServerTime, auto pExecTime, auto pSymbol, auto pOrderType, auto pPrice, auto pShareSize) {
            cout << (rpt1or2 ? "Report1: " : "Report2: ")
                 << *pUserName << "\t"
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
                *pUserName1,
                *pSymbol,
                orderStatus,
                *pOrderID1,
                *pOrderType1,
                *pShareSize,
                *pLeftQty,
                *pPrice,
                *pDestination,
                pServerTime->getValue(),
                pExecTime->getValue()
            };

            if (FIX::ExecType_FILL == orderStatus) { // TRADE
                Report report1{
                    *pUserName1,
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
                    *pUserName2,
                    *pOrderID2,
                    orderStatus,
                    *pSymbol,
                    *pOrderType2,
                    *pPrice,
                    *pShareSize,
                    pServerTime->getValue(),
                    pExecTime->getValue()
                };

                printRpts(true, pUserName1, pOrderID1, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType1, pPrice, pShareSize);
                printRpts(false, pUserName2, pOrderID2, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType2, pPrice, pShareSize);

                BCDocuments* docs = BCDocuments::getInstance();
                docs->addCandleToSymbol(*pSymbol, transac);
                docs->addReportToUserRiskManagement(*pUserName1, report1);
                docs->addReportToUserRiskManagement(*pUserName2, report2);
            } else { // TRTH TRADE
                BCDocuments::getInstance()->addCandleToSymbol(*pSymbol, transac);
            }

            FIXAcceptor::getInstance()->sendLatestStockPrice2All(transac);
        } break;
        case FIX::ExecType_EXPIRED: { // CANCEL
            Report report2{
                *pUserName2,
                *pOrderID2,
                orderStatus,
                *pSymbol,
                *pOrderType2,
                *pPrice,
                *pShareSize,
                pServerTime->getValue(),
                pExecTime->getValue()
            };

            BCDocuments::getInstance()->addReportToUserRiskManagement(*pUserName2, report2);
            printRpts(true, pUserName1, pOrderID1, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType1, pPrice, pShareSize);
            printRpts(false, pUserName2, pOrderID2, orderStatus, pServerTime, pExecTime, pSymbol, pOrderType2, pPrice, pShareSize);
        } break;
        } // switch

        if (prevCnt) { // > 1 threads
            delete pSymbol;
            delete pOrderID1;
            delete pOrderID2;
            delete pUserName1;
            delete pUserName2;
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
void FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoMDEntries numOfEntry;
    message.get(numOfEntry);
    if (numOfEntry < 1) {
        cout << "Cannot find the entry group in messge !" << endl;
        return;
    }

    static FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    static FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs partyGroup;

    static FIX::MDEntryType bookType;
    static FIX::Symbol symbol;
    static FIX::MDEntryPx price;
    static FIX::MDEntrySize size;
    static FIX::Text time;
    static FIX::PartyID destination;
    static FIX::ExpireTime utctime;

    // #pragma GCC diagnostic ignored ....

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries* pEntryGroup;
    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs* pPartyGroup;

    FIX::MDEntryType* pBookType;
    FIX::Symbol* pSymbol;
    FIX::MDEntryPx* pPrice;
    FIX::MDEntrySize* pSize;
    FIX::Text* pTime;
    FIX::PartyID* pDestination;
    FIX::ExpireTime* pUtctime;

    static std::atomic<unsigned int> s_cntAtom{ 0 };
    unsigned int prevCnt = 0;

    while (!s_cntAtom.compare_exchange_strong(prevCnt, prevCnt + 1))
        ;
    assert(s_cntAtom > 0);

    if (0 == prevCnt) { // sequential case; optimized:
        pEntryGroup = &entryGroup;
        pPartyGroup = &partyGroup;
        pBookType = &bookType;
        pSymbol = &symbol;
        pPrice = &price;
        pSize = &size;
        pTime = &time;
        pDestination = &destination;
        pUtctime = &utctime;
    } else { // > 1 threads; always safe way:
        pEntryGroup = new decltype(entryGroup);
        pPartyGroup = new decltype(partyGroup);
        pBookType = new decltype(bookType);
        pSymbol = new decltype(symbol);
        pPrice = new decltype(price);
        pSize = new decltype(size);
        pTime = new decltype(time);
        pDestination = new decltype(destination);
        pUtctime = new decltype(utctime);
    }

    message.getGroup(1, *pEntryGroup);
    pEntryGroup->get(*pBookType);
    pEntryGroup->get(*pSymbol);
    pEntryGroup->get(*pPrice);
    pEntryGroup->get(*pSize);
    pEntryGroup->get(*pTime);
    pEntryGroup->get(*pUtctime);

    pEntryGroup->getGroup(1, *pPartyGroup);
    pPartyGroup->get(*pDestination);

    // cout << "Test Use: " << pTime->getString() << " " << pUtctime->getString() << endl;

    OrderBookEntry update{
        OrderBookEntry::s_toOrderBookType(*pBookType),
        pSymbol->getString(),
        double{ *pPrice },
        double{ *pSize },
        double{ std::stod(pTime->getString()) },
        pDestination->getString(),
        pUtctime->getValue()
    };
    BCDocuments::getInstance()->addStockToSymbol(*pSymbol, update);

    if (prevCnt) { // > 1 threads
        delete pEntryGroup;
        delete pPartyGroup;
        delete pBookType;
        delete pSymbol;
        delete pPrice;
        delete pSize;
        delete pTime;
        delete pDestination;
        delete pUtctime;
    }

    s_cntAtom--;
    assert(s_cntAtom >= 0);
}

/*
 * @brief Receive security list from ME
 */
void FIXInitiator::onMessage(const FIX50SP2::SecurityList& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoRelatedSym numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find Symbol in SecurityList from ME" << endl;
    }

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    FIX::Symbol symbol;

    for (int i = 1; i <= numOfGroup; i++) {
        message.getGroup(i, relatedSymGroup);
        relatedSymGroup.get(symbol);
        BCDocuments::getInstance()->addSymbol(symbol);
        OrderBookEntry update(OrderBookEntry::s_toOrderBookType('e'),
            "initial",
            0.0,
            0.0,
            0.0,
            "initial",
            FIX::UtcTimeStamp(6)); // FIXME: not used
        BCDocuments::getInstance()->addStockToSymbol(symbol, update);
    }
}
