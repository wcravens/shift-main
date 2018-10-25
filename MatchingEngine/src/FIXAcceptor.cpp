#include "FIXAcceptor.h"

#include "Stock.h"
#include "globalvariables.h"

#include <map>

#include <shift/miscutils/crossguid/Guid.h>
#include <shift/miscutils/terminal/Common.h>

extern std::map<std::string, Stock> stocklist;

/* static */ std::string FIXAcceptor::s_senderID;
/* static */ std::list<std::string> FIXAcceptor::clientlist;
/* static */ std::mutex FIXAcceptor::clientid_mutex;
/* static */ std::mutex FIXAcceptor::ExecReport_mutex;

// Predefined constant FIX fields (To avoid recalculations):
static const auto& FIXFIELD_BEGSTR = FIX::BeginString("FIXT.1.1"); // FIX BeginString
static const auto& FIXFIELD_MDUPDATE_CHANGE = FIX::MDUpdateAction('1');
static const auto& FIXFIELD_EXECBROKER = FIX::PartyRole(1); // 1 = ExecBroker in FIX4.2
static const auto& FIXFIELD_EXECTYPE_TRADE = FIX::ExecType('F'); // F = Trade
static const auto& FIXFIELD_LEAVQTY_0 = FIX::LeavesQty(0); // Quantity open for further execution
static const auto& FIXFIELD_CLIENTID = FIX::PartyRole(3); // 3 = Client ID in FIX4.2
static const auto& FIXFIELD_EXECTYPE_NEW = FIX::ExecType('0'); // 0 = New
static const auto& FIXFIELD_CUMQTY_0 = FIX::CumQty(0);

FIXAcceptor::~FIXAcceptor() // override
{
    disconnectBrokerageCenter();
}

void FIXAcceptor::connectBrokerageCenter(const std::string& configFile)
{
    disconnectBrokerageCenter();

    FIX::SessionSettings settings(configFile);
    FIX::Dictionary commonDict = settings.get();

    if (commonDict.has("FileLogPath")) {
        m_logFactoryPtr.reset(new FIX::FileLogFactory(commonDict.getString("FileLogPath")));
    } else {
        m_logFactoryPtr.reset(new FIX::ScreenLogFactory(false, false, true));
    }

    if (commonDict.has("FileStorePath")) {
        m_messageStoreFactoryPtr.reset(new FIX::FileStoreFactory(commonDict.getString("FileStorePath")));
    } else {
        m_messageStoreFactoryPtr.reset(new FIX::NullStoreFactory());
    }

    m_socketAcceptorPtr.reset(new FIX::SocketAcceptor(*this, *m_messageStoreFactoryPtr, settings, *m_logFactoryPtr));

    cout << '\n'
         << COLOR "Acceptor is starting..." NO_COLOR << '\n'
         << endl;

    try {
        m_socketAcceptorPtr->start();
    } catch (const FIX::RuntimeError& e) {
        cout << COLOR_ERROR << e.what() << NO_COLOR << endl;
    }
}

void FIXAcceptor::disconnectBrokerageCenter()
{
    if (!m_socketAcceptorPtr)
        return;

    cout << '\n'
         << COLOR "Acceptor is stopping..." NO_COLOR << '\n'
         << flush;

    m_socketAcceptorPtr->stop();
    m_socketAcceptorPtr = nullptr;
    m_messageStoreFactoryPtr = nullptr;
    m_logFactoryPtr = nullptr;
}

/*
 * @brief Send security list to BC
 */
void FIXAcceptor::sendSecurityList(const std::string& clientID)
{
    FIX50SP2::SecurityList message;
    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_SecurityList));

    message.setField(FIX::SecurityResponseID(shift::crossguid::newGuid().str()));

    FIX50SP2::SecurityList::NoRelatedSym relatedSymGroup;

    for (auto iter = symbollist.begin(); iter != symbollist.end(); iter++) {
        relatedSymGroup.set(FIX::Symbol(*iter));
        message.addGroup(relatedSymGroup);
    }

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send order book update to client
 */
/* static */ void FIXAcceptor::SendOrderbookUpdate(std::string& clientID, Newbook& update)
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(clientID));
    header.setField(FIX::MsgType(FIX::MsgType_MarketDataIncrementalRefresh));

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries entryGroup;
    entryGroup.setField(::FIXFIELD_MDUPDATE_CHANGE); // Required by FIX
    entryGroup.setField(FIX::MDEntryType(update.getbook()));
    entryGroup.setField(FIX::Symbol(update.getsymbol()));
    entryGroup.setField(FIX::MDEntryPx(update.getprice()));
    entryGroup.setField(FIX::MDEntrySize(update.getsize()));
    entryGroup.setField(FIX::Text(std::to_string(update.gettime()))); // FIXME: Use FIX::UTCTimestamp instead

    FIX50SP2::MarketDataIncrementalRefresh::NoMDEntries::NoPartyIDs partyGroup;
    partyGroup.setField(FIXFIELD_EXECBROKER);
    partyGroup.setField(FIX::PartyID(update.getdestination()));
    entryGroup.addGroup(partyGroup);

    message.addGroup(entryGroup);

    FIX::Session::sendToTarget(message);
}

/*
 * @brief Send order book update to all client
 */
/* static */ void FIXAcceptor::SendOrderbookUpdate2All(Newbook& newbook)
{
    std::lock_guard<std::mutex> lock(clientid_mutex);
    std::for_each(clientlist.begin(), clientlist.end(), std::bind(&FIXAcceptor::SendOrderbookUpdate, std::placeholders::_1, newbook));
}

/**
 * @brief Sending the execution report to the client
 */
/* static */ void FIXAcceptor::SendExecution2client(std::string& targetID, action& actions)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID(targetID));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(actions.order_id1));
    message.setField(FIX::SecondaryOrderID(actions.order_id2));
    message.setField(::FIXFIELD_EXECTYPE_TRADE); // Required by FIX
    message.setField(FIX::OrdStatus(actions.decision));
    message.setField(FIX::Symbol(actions.stockname));
    message.setField(FIX::Side(actions.order_type1));
    message.setField(FIX::OrdType(actions.order_type2));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_LEAVQTY_0); // Required by FIX
    message.setField(FIX::CumQty(actions.size));
    message.setField(FIX::Price(actions.price));

    message.setField(FIX::TransactTime(6));
    // message.setField(FIX::Text(actions.exetime)); // FIXME: Use FIX::EffectiveTime instead
    message.setField(FIX::EffectiveTime(actions.utc_exetime, 6));

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup1;
    idGroup1.set(::FIXFIELD_CLIENTID);
    idGroup1.set(FIX::PartyID(actions.trader_id1));
    message.addGroup(idGroup1);

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup2;
    idGroup2.set(::FIXFIELD_CLIENTID);
    idGroup2.set(FIX::PartyID(actions.trader_id2));
    message.addGroup(idGroup2);

    FIX50SP2::ExecutionReport::NoPartyIDs brokerGroup;
    brokerGroup.set(::FIXFIELD_EXECBROKER);
    brokerGroup.set(FIX::PartyID(actions.destination));
    message.addGroup(brokerGroup);

    FIX::Session::sendToTarget(message);
}

/**
 * @brief Sending the execution report to all clients
 */
/* static */ void FIXAcceptor::SendExecution(const action& actions)
{
    std::lock_guard<std::mutex> lock1(ExecReport_mutex);
    std::lock_guard<std::mutex> lock2(clientid_mutex);
    std::for_each(clientlist.begin(), clientlist.end(), std::bind(&FIXAcceptor::SendExecution2client, std::placeholders::_1, actions));
}

void FIXAcceptor::onCreate(const FIX::SessionID& sessionID) // override
{
    s_senderID = sessionID.getSenderCompID();
}

void FIXAcceptor::onLogon(const FIX::SessionID& sessionID) // override
{
    std::string clientid = sessionID.getTargetCompID();
    cout << "ID: " << clientid << "  logon" << endl;
    std::list<std::string>::iterator newID;
    // Check whether clientID already exist in clientidlist
    newID = find(clientlist.begin(), clientlist.end(), clientid);
    if (newID == clientlist.end()) {
        std::lock_guard<std::mutex> lock(clientid_mutex);
        clientlist.push_back(clientid);
        cout << "Add new clientID: " << clientid << endl;
    }
    // Send stock list to client
    sendSecurityList(clientid);
}

void FIXAcceptor::fromApp(const FIX::Message& message,
    const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) // override
{
    crack(message, sessionID);
}

/**
 * @brief Deal with the incoming messages which type are NewOrderSingle
 */
void FIXAcceptor::onMessage(const FIX50SP2::NewOrderSingle& message, const FIX::SessionID& sessionID) // override
{
    FIX::NoPartyIDs numOfGroup;
    message.get(numOfGroup);
    if (numOfGroup < 1) {
        cout << "Cannot find ClientID in NewOrderSingle!" << endl;
        return;
    }

    FIX::ClOrdID orderID;
    FIX::Symbol symbol;
    FIX::OrderQty orderQty;
    FIX::OrdType ordType;
    FIX::Price price;

    message.get(orderID);
    message.get(symbol);
    message.get(orderQty);
    message.get(ordType);
    message.get(price);

    FIX50SP2::NewOrderSingle::NoPartyIDs idGroup;
    FIX::PartyID clientID;
    message.getGroup(1, idGroup);
    idGroup.get(clientID);

    // If order size is less than 0, reject the quote
    if (orderQty <= 0) {
        QuoteConfirm rejection = { 
            clientID,
            orderID, 
            symbol, 
            price, 
            0, 
            ordType, 
            // timepara.timestamp_inner(), 
            timepara.timestamp_now()
        };
        sendQuoteConfirmation(rejection);
        return;
    } else {
        std::string tmp2 = "XX"; // Define themp2 to output the type of the order
        switch (ordType) {
        case '1': {
            cout << "Received Order From " << clientID << endl;
            tmp2 = "Limit Buy";
            break;
        }
        case '2': {
            cout << "Received Order From " << clientID << endl;
            tmp2 = "Limit Sell";
            break;
        }
        case '3': {
            cout << "Received Order From " << clientID << endl;
            tmp2 = "Market Buy";
            break;
        }
        case '4': {
            cout << "Received Order From " << clientID << endl;
            tmp2 = "Market Sell";
            break;
        }
        case '5': {
            cout << "Received Order Cancel Request From " << clientID << endl;
            tmp2 = "Cancel Bid";
            break;
        }
        case '6': {
            cout << "Received Order Cancel Request From " << clientID << endl;
            tmp2 = "Cancel Ask";
            break;
        }
        }

        double millidiffer = timepara.past_milli();
        std::string timestring = timepara.milli2str(millidiffer);
        
        double milli = timepara.past_milli();
        FIX::UtcTimeStamp utc_now = timepara.milli2utc(milli);
        
        cout << "test use: " << millidiffer << " " << milli << endl;

        // std::string timestring = timepara.milli2str(millidiffer);
        // cout << "Quote receive in innertime :  " << timestring << endl;
        // auto utc_now = timepara.timestamp_now();

        // auto tmp = FIX::TransactTime(utc_now, 6);
        // cout << "Test Use: " << tmp.getString() << endl;

        Quote quote(symbol, clientID, orderID, price, orderQty, ordType, utc_now);

        quote.setmili(milli);
        // Input the newquote into buffer
        std::map<std::string, Stock>::iterator stockIt;
        stockIt = stocklist.find(symbol);
        if (stockIt != stocklist.end()) {
            stockIt->second.buf_new_local(quote);
        } else if (stockIt == stocklist.end()) {
            orderQty = 0;
        }

        // Send confirmation to client
        QuoteConfirm confirmation = {clientID, orderID, symbol, price, (int)orderQty, ordType, utc_now};
        sendQuoteConfirmation(confirmation);

        cout << "Sending the confirmation:  " << orderID << endl;
    }
}

/**
 * @brief Send the order confirmation to the client, 
 * because report.status usual set to 1. Then the 
 * client will notify it is a confirmation
 */
void FIXAcceptor::sendQuoteConfirmation(QuoteConfirm& confirm)
{
    FIX::Message message;

    FIX::Header& header = message.getHeader();
    header.setField(::FIXFIELD_BEGSTR);
    header.setField(FIX::SenderCompID(s_senderID));
    header.setField(FIX::TargetCompID("BROKERAGECENTER"));
    header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));

    message.setField(FIX::OrderID(confirm.orderID));
    message.setField(FIX::ExecID(shift::crossguid::newGuid().str()));
    message.setField(::FIXFIELD_EXECTYPE_NEW); // Required by FIX
    message.setField(FIX::OrdStatus(FIX::OrdStatus_NEW));
    message.setField(FIX::Symbol(confirm.symbol));
    message.setField(FIX::Side(confirm.ordertype));
    message.setField(FIX::Price(confirm.price));
    message.setField(FIX::LeavesQty(confirm.size));
    message.setField(::FIXFIELD_CUMQTY_0); // Required by FIX
    message.setField(FIX::TransactTime(6));
    message.setField(FIX::EffectiveTime(confirm.utc_time, 6));
    // message.setField(FIX::Text(confirm.time)); // FIXME: Use FIX::EffectiveTime instead

    FIX50SP2::ExecutionReport::NoPartyIDs idGroup;
    idGroup.set(::FIXFIELD_CLIENTID);
    idGroup.set(FIX::PartyID(confirm.clientID));
    message.addGroup(idGroup);

    FIX::Session::sendToTarget(message);
}

/*void FIXAcceptor::SendExecutionReport(action actions)    //Sending the execution report to the client
{
    FIX::Message message;
    FIX::Header& header = message.getHeader();

    if(actions.decision == '2')
    {

/////////if this is a trade, send execution report to two clients, and store the record into Database
/////////send to client1
        if(actions.trader_id1!="TR")
        {
            header.setField(FIX::BeginString("FIX.4.2"));
            header.setField(FIX::SenderCompID("EXECUTOR"));
            header.setField(FIX::TargetCompID(actions.trader_id1));
            header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));
            message.setField(FIX::Symbol(actions.stockname));
            message.setField(FIX::OrdStatus(actions.decision));
            message.setField(FIX::OrderID(actions.order_id1));
            message.setField(FIX::LeavesQty(actions.size));//I dont know whether I should change this line.
            message.setField(FIX::CumQty(actions.size));
            message.setField(FIX::AvgPx(actions.price));//added by chang'an
            message.setField(FIX::Text(now_str())); // Millisecond
            message.setField(FIX::TransactTime()); //date
            message.setField(FIX::ClOrdID(actions.order_id1));
            message.setField(FIX::ExecID(actions.exetime));
            message.setField(FIX::ExecBroker(actions.destination));
            message.setField(FIX::ExecType(actions.order_type1));

            //message useless below:

            message.setField(FIX::ExecTransType('0'));
            message.setField(FIX::Side('1'));


            FIX::Session::sendToTarget(message);
            cout<<endl<<"Sending Execution Report to "<<actions.trader_id1<<endl;
        }
        /////send to client2
        if(actions.trader_id2!="TR")
        {


            header.setField(FIX::BeginString("FIX.4.2"));
            header.setField(FIX::SenderCompID("EXECUTOR"));
            header.setField(FIX::TargetCompID(actions.trader_id2));
            header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));
            message.setField(FIX::Symbol(actions.stockname));
            message.setField(FIX::OrdStatus(actions.decision));
            message.setField(FIX::OrderID(actions.order_id2));
            message.setField(FIX::LeavesQty(100));
            message.setField(FIX::CumQty(actions.size));
            message.setField(FIX::AvgPx(actions.price));
            message.setField(FIX::Text(now_str())); // Millisecond
            message.setField(FIX::TransactTime()); //date
            message.setField(FIX::ClOrdID(actions.order_id2));
            message.setField(FIX::ExecID(actions.exetime));
            message.setField(FIX::ExecBroker(actions.destination));
            message.setField(FIX::ExecType(actions.order_type2));
            //message useless below:

            message.setField(FIX::ExecTransType('0'));
            message.setField(FIX::Side('1'));



            FIX::Session::sendToTarget(message);
            cout<<endl<<"Sending Execution Report to "<<actions.trader_id2<<endl;
        }
    }

    if(actions.decision == 'C')
    {
        /////////////if it is a cancel,send the execution report
        //To Client:
        header.setField(FIX::BeginString("FIX.4.2"));
        header.setField(FIX::SenderCompID("EXECUTOR"));
        header.setField(FIX::TargetCompID(actions.trader_id1));
        header.setField(FIX::MsgType(FIX::MsgType_ExecutionReport));
        message.setField(FIX::OrdStatus(actions.decision));
        message.setField(FIX::OrderID(actions.order_id1));
        message.setField(FIX::LeavesQty(100));
        message.setField(FIX::CumQty(actions.size));
        message.setField(FIX::Text(now_str())); // Millisecond
        message.setField(FIX::TransactTime()); //date
        message.setField(FIX::ClOrdID(actions.order_id1));
        message.setField(FIX::ExecID(actions.exetime));
        message.setField(FIX::Symbol(actions.destination));
        //message useless below:
        message.setField(FIX::ExecType('1'));
        message.setField(FIX::ExecTransType('0'));
        message.setField(FIX::Side('1'));
        message.setField(FIX::AvgPx(40)); //report.getAvgPx()


        FIX::Session::sendToTarget(message);
        cout<<endl<<"Sending Execution Report(cancelling) to "<<actions.trader_id1<<endl;
    }
}
*/
