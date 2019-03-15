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
 * @brief Deal with incoming messages which type is Execution Report
 */
void FIXInitiator::onMessage(const FIX50SP2::ExecutionReport& message, const FIX::SessionID&) // override
{
    FIX::ExecType execType;
    message.get(execType);

    if (execType == FIX::ExecType_ORDER_STATUS) { // Confirmation Report
        // Update Version: ClientID has been replaced by PartyRole and PartyID
        FIX::NoPartyIDs numOfGroups;
        message.get(numOfGroups);
        if (numOfGroups < 1) {
            cout << "Cannot find any ClientID in ExecutionReport!" << endl;
            return;
        }

        FIX::OrderID orderID;
        FIX::OrdStatus orderStatus;
        FIX::Symbol orderSymbol;
        FIX::Side orderType;
        FIX::Price orderPrice;
        FIX::EffectiveTime confirmTime;
        FIX::LastMkt destination;
        FIX::LeavesQty currentSize;
        FIX::TransactTime serverTime;

        FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        FIX::PartyID username;

        message.get(orderID);
        message.get(orderStatus);
        message.get(orderSymbol);
        message.get(orderType);
        message.get(orderPrice);
        message.get(confirmTime);
        message.get(destination);
        message.get(currentSize);
        message.get(serverTime);

        message.getGroup(1, idGroup);
        idGroup.get(username);

        Report report{
            username.getValue(),
            orderID.getValue(),
            static_cast<Order::Type>(static_cast<char>(orderType.getValue())),
            orderSymbol.getValue(),
            static_cast<int>(currentSize.getValue()),
            0, // executed size
            orderPrice.getValue(),
            static_cast<Order::Status>(static_cast<char>(orderStatus.getValue())),
            destination.getValue(),
            serverTime.getValue(),
            confirmTime.getValue()
        };

        cout << "ConfirmRepo: "
             << username.getValue() << "\t"
             << orderID.getValue() << "\t"
             << orderType.getValue() << "\t"
             << orderSymbol.getValue() << "\t"
             << currentSize.getValue() << "\t"
             << orderPrice.getValue() << "\t"
             << orderStatus.getValue() << "\t"
             << destination.getValue() << "\t"
             << serverTime.getString() << "\t"
             << confirmTime.getString() << endl;
        BCDocuments::getInstance()->onNewReportForUserRiskManagement(username, report);

    } else { // FIX::ExecType_TRADE: Execution Report
        // Update Version: ClientID has been replaced by PartyRole and PartyID
        FIX::NoPartyIDs numOfGroups;
        message.get(numOfGroups);
        if (numOfGroups < 2) {
            cout << "Cannot find all ClientID in ExecutionReport!" << endl;
            return;
        }

        FIX::OrderID orderID1;
        FIX::SecondaryOrderID orderID2;
        FIX::OrdStatus orderStatus;
        FIX::Symbol orderSymbol;
        FIX::Side orderType1;
        FIX::OrdType orderType2;
        FIX::Price orderPrice;
        FIX::EffectiveTime execTime;
        FIX::LastMkt destination;
        FIX::CumQty executedSize;
        FIX::TransactTime serverTime;

        FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
        FIX::PartyID username1;
        FIX::PartyID username2;

        message.get(orderID1);
        message.get(orderID2);
        message.get(orderStatus);
        message.get(orderSymbol);
        message.get(orderType1);
        message.get(orderType2);
        message.get(orderPrice);
        message.get(execTime);
        message.get(destination);
        message.get(executedSize);
        message.get(serverTime);

        message.getGroup(1, idGroup);
        idGroup.get(username1);
        message.getGroup(2, idGroup);
        idGroup.get(username2);

        auto printRpts = [](bool rpt1or2, auto username, auto orderID, auto orderType, auto orderSymbol, auto executedSize, auto orderPrice, auto orderStatus, auto destination, auto serverTime, auto execTime) {
            cout << (rpt1or2 ? "Report1: " : "Report2: ")
                 << username.getValue() << "\t"
                 << orderID.getValue() << "\t"
                 << orderType.getValue() << "\t"
                 << orderSymbol.getValue() << "\t"
                 << executedSize.getValue() << "\t"
                 << orderPrice.getValue() << "\t"
                 << orderStatus.getValue() << "\t"
                 << destination.getValue() << "\t"
                 << serverTime.getString() << "\t"
                 << execTime.getString() << endl;
        };

        switch (orderStatus) {
        case FIX::OrdStatus_FILLED:
        case FIX::OrdStatus_REPLACED: {
            Transaction transac = {
                orderSymbol.getValue(),
                static_cast<int>(executedSize.getValue()),
                orderPrice.getValue(),
                destination.getValue(),
                execTime.getValue()
            };

            if (FIX::OrdStatus_FILLED == orderStatus) { // TRADE
                Report report1{
                    username1.getValue(),
                    orderID1.getValue(),
                    static_cast<Order::Type>(static_cast<char>(orderType1.getValue())),
                    orderSymbol.getValue(),
                    0, // current size: will be added later
                    static_cast<int>(executedSize.getValue()),
                    orderPrice.getValue(),
                    static_cast<Order::Status>(static_cast<char>(orderStatus.getValue())),
                    destination.getValue(),
                    serverTime.getValue(),
                    execTime.getValue()
                };

                Report report2{
                    username2.getValue(),
                    orderID2.getValue(),
                    static_cast<Order::Type>(static_cast<char>(orderType2.getValue())),
                    orderSymbol.getValue(),
                    0, // current size: will be added later
                    static_cast<int>(executedSize.getValue()),
                    orderPrice.getValue(),
                    static_cast<Order::Status>(static_cast<char>(orderStatus.getValue())),
                    destination.getValue(),
                    serverTime.getValue(),
                    execTime.getValue()
                };

                printRpts(true, username1, orderID1, orderType1, orderSymbol, executedSize, orderPrice, orderStatus, destination, serverTime, execTime);
                printRpts(false, username2, orderID2, orderType2, orderSymbol, executedSize, orderPrice, orderStatus, destination, serverTime, execTime);

                auto* docs = BCDocuments::getInstance();
                docs->onNewTransacForCandlestickData(orderSymbol, transac);
                docs->onNewReportForUserRiskManagement(username1.getValue(), report1);
                docs->onNewReportForUserRiskManagement(username2.getValue(), report2);
            } else { // FIX::OrdStatus_REPLACED: TRTH TRADE
                BCDocuments::getInstance()->onNewTransacForCandlestickData(orderSymbol.getValue(), transac);
            }

            FIXAcceptor::getInstance()->sendLastPrice2All(transac);
        } break;
        case FIX::OrdStatus_CANCELED: { // CANCELLATION
            Report report2{
                username2.getValue(),
                orderID2.getValue(),
                static_cast<Order::Type>(static_cast<char>(orderType2.getValue())),
                orderSymbol.getValue(),
                0, // current size: will be added later
                static_cast<int>(executedSize.getValue()),
                orderPrice.getValue(),
                static_cast<Order::Status>(static_cast<char>(orderStatus.getValue())),
                destination.getValue(),
                serverTime.getValue(),
                execTime.getValue()
            };

            printRpts(true, username1, orderID1, orderType1, orderSymbol, executedSize, orderPrice, orderStatus, destination, serverTime, execTime);
            printRpts(false, username2, orderID2, orderType2, orderSymbol, executedSize, orderPrice, orderStatus, destination, serverTime, execTime);

            BCDocuments::getInstance()->onNewReportForUserRiskManagement(username2.getValue(), report2);
        } break;
        } // switch
    }
}

/**
 * @brief Receive order book updates
 */
void FIXInitiator::onMessage(const FIX50SP2::MarketDataIncrementalRefresh& message, const FIX::SessionID&) // override
{
    FIX::NoMDEntries numOfEntries;
    message.get(numOfEntries);
    if (numOfEntries < 1) {
        cout << "Cannot find the Entries group in MarketDataIncrementalRefresh!" << endl;
        return;
    }

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;

    FIX::MDEntryType bookType;
    FIX::Symbol symbol;
    FIX::MDEntryPx price;
    FIX::MDEntrySize size;
    FIX::MDEntryDate date;
    FIX::MDEntryTime daytime;
    FIX::MDMkt destination;

    message.getGroup(1, entryGroup);
    entryGroup.get(bookType);
    entryGroup.get(symbol);
    entryGroup.get(price);
    entryGroup.get(size);
    entryGroup.get(date);
    entryGroup.get(daytime);
    entryGroup.get(destination);

    OrderBookEntry update{
        static_cast<OrderBookEntry::Type>(static_cast<char>(bookType.getValue())),
        symbol.getValue(),
        price.getValue(),
        static_cast<int>(size.getValue()),
        destination.getValue(),
        date.getValue(),
        daytime.getValue()
    };

    BCDocuments::getInstance()->onNewOBEntryForOrderBook(symbol.getValue(), update);
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

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;
    FIX::Symbol symbol;

    auto* docs = BCDocuments::getInstance();
    for (int i = 1; i <= numOfGroups.getValue(); i++) {
        message.getGroup(static_cast<unsigned int>(i), relatedSymGroup);
        relatedSymGroup.get(symbol);

        docs->addSymbol(symbol.getValue());
        docs->attachOrderBookToSymbol(symbol.getValue());
        docs->attachCandlestickDataToSymbol(symbol.getValue());
    }

    // Now, it's safe to advance all routines that *read* permanent data structures created above:
    BCDocuments::s_isSecurityListReady = true;
}
