#include "RiskManagement.h"

#include "BCDocuments.h"
#include "DBConnector.h"
#include "FIXAcceptor.h"
#include "FIXInitiator.h"

#include <cmath>

#include <shift/miscutils/concurrency/Consumer.h>
#include <shift/miscutils/terminal/Common.h>

/* static */ inline void RiskManagement::s_sendOrderToME(const Order& order)
{
    FIXInitiator::getInstance()->sendOrder(order);
}

/* static */ inline void RiskManagement::s_sendPortfolioSummaryToUser(const std::string& userID, const PortfolioSummary& summary)
{
    FIXAcceptor::getInstance()->sendPortfolioSummary(userID, summary);
}

/* static */ inline void RiskManagement::s_sendPortfolioItemToUser(const std::string& userID, const PortfolioItem& item)
{
    FIXAcceptor::getInstance()->sendPortfolioItem(userID, item);
}

RiskManagement::RiskManagement(const std::string& userID, double buyingPower)
    : m_userID(userID)
    , m_porfolioSummary(buyingPower)
    , m_pendingShortCashAmount(0.0)
{
    // create "empty" waiting list:
    // this is required to be able to send empty waiting lists to clients,
    // which will ignore any orders in the waiting list with size 0
    m_waitingList["0"] = { Order::Type::LIMIT_BUY, "0", 0, 0.0, "0", userID };
}

RiskManagement::RiskManagement(const std::string& userID, double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares)
    : m_userID(userID)
    , m_porfolioSummary(buyingPower, holdingBalance, borrowedBalance, totalPL, totalShares)
    , m_pendingShortCashAmount(0.0)
{
    // create "empty" waiting list:
    // this is required to be able to send empty waiting lists to clients,
    // which will ignore any orders in the waiting list with size 0
    m_waitingList["0"] = { Order::Type::LIMIT_BUY, "0", 0, 0.0, "0", userID };
}

RiskManagement::~RiskManagement()
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlagExec, m_cvExecRpt, *m_execRptThread);
    m_execRptThread = nullptr;
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlagOrder, m_cvOrder, *m_orderThread);
    m_orderThread = nullptr;
}

void RiskManagement::spawn()
{
    m_orderThread.reset(new std::thread(&RiskManagement::processOrder, this));
    m_execRptThread.reset(new std::thread(&RiskManagement::processExecRpt, this));
}

inline double RiskManagement::getMarketBuyPrice(const std::string& symbol)
{
    return BCDocuments::getInstance()->getOrderBookMarketFirstPrice(true, symbol);
}

inline double RiskManagement::getMarketSellPrice(const std::string& symbol)
{
    return BCDocuments::getInstance()->getOrderBookMarketFirstPrice(false, symbol);
}

void RiskManagement::insertPortfolioItem(const std::string& symbol, const PortfolioItem& portfolioItem)
{
    std::lock_guard<std::mutex> guard(m_mtxPortfolioItems);
    m_portfolioItems[symbol] = portfolioItem;
}

void RiskManagement::sendPortfolioHistory()
{
    std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
    std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);

    s_sendPortfolioSummaryToUser(m_userID, m_porfolioSummary);

    for (auto& i : m_portfolioItems)
        s_sendPortfolioItemToUser(m_userID, i.second);
}

void RiskManagement::updateWaitingList(const ExecutionReport& report)
{
    std::lock_guard<std::mutex> guard(m_mtxWaitingList);

    auto it = m_waitingList.find(report.orderID);
    if (m_waitingList.end() == it)
        return;

    auto& order = it->second;
    order.setExecutedSize(order.getExecutedSize() + report.executedSize);
    if (order.getExecutedSize() == order.getSize()) {
        m_waitingList.erase(it);
    } else {
        if (report.orderStatus == Order::Status::CANCELED) {
            order.setStatus(Order::Status::PARTIALLY_FILLED);
        } else {
            order.setStatus(report.orderStatus);
        }
    }
}

void RiskManagement::sendWaitingList() const
{
    std::lock_guard<std::mutex> guard(m_mtxWaitingList);
    if (!m_waitingList.empty())
        FIXAcceptor::getInstance()->sendWaitingList(m_userID, m_waitingList);
}

void RiskManagement::enqueueOrder(Order&& order)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxOrder);
        m_orderBuffer.push(std::move(order));
    }
    m_cvOrder.notify_one();
}

void RiskManagement::processOrder()
{
    thread_local auto quitFut = m_quitFlagOrder.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxOrder);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvOrder, lock, [this] { return !m_orderBuffer.empty(); }))
            return;

        auto orderPtr = &m_orderBuffer.front();

        if (m_portfolioItems.find(orderPtr->getSymbol()) == m_portfolioItems.end()) { // add new portfolio item ?
            insertPortfolioItem(orderPtr->getSymbol(), orderPtr->getSymbol());

            if (!DBConnector::s_isPortfolioDBReadOnly) {
                auto lock { DBConnector::getInstance()->lockPSQL() };
                DBConnector::getInstance()->doQuery("INSERT INTO portfolio_items (id, symbol) VALUES ('" + m_userID + "','" + orderPtr->getSymbol() + "');", "");
            }
        }

        if (verifyAndSendOrder(*orderPtr)) {
            {
                std::lock_guard<std::mutex> guard(m_mtxWaitingList);
                if (orderPtr->getType() != Order::Type::CANCEL_BID && orderPtr->getType() != Order::Type::CANCEL_ASK) {
                    m_waitingList[orderPtr->getID()] = *orderPtr;
                }
            }

            sendWaitingList();
        }

        orderPtr = nullptr;
        m_orderBuffer.pop();
    }
}

void RiskManagement::enqueueExecRpt(ExecutionReport&& report)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxExecRpt);
        m_execRptBuffer.push(std::move(report));
    }
    m_cvExecRpt.notify_one();
}

void RiskManagement::processExecRpt()
{
    thread_local auto quitFut = m_quitFlagExec.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxExecRpt);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvExecRpt, lock, [this] { return !m_execRptBuffer.empty(); }))
            return;

        auto reportPtr = &m_execRptBuffer.front();

        // if it is not a confirmation report
        if (reportPtr->orderStatus != Order::Status::NEW && reportPtr->orderStatus != Order::Status::PENDING_CANCEL) {

            if (reportPtr->orderStatus == Order::Status::FILLED) { // execution report

                if (reportPtr->orderType == Order::Type::MARKET_BUY || reportPtr->orderType == Order::Type::LIMIT_BUY) {

                    std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                    std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
                    std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

                    auto& item = m_portfolioItems[reportPtr->orderSymbol];

                    const double price = reportPtr->orderPrice; // NP
                    const int buyShares = reportPtr->executedSize * 100; // NS

                    double inc = 0.0;

                    if (item.getShortShares() == 0) { // no current short positions
                        item.addLongPrice(price, buyShares);
                        item.addLongShares(buyShares);
                    } else {
                        if (buyShares > item.getShortShares()) {
                            inc = (item.getShortPrice() - price) * item.getShortShares(); // (OP - NP) * OS

                            // all short positions are "consumed" and withholded money is returned
                            m_porfolioSummary.returnBalance(item.getBorrowedBalance());
                            item.resetBorrowedBalance();

                            double rem = buyShares - item.getShortShares();
                            item.addLongPrice(price, rem);
                            item.addLongShares(rem);
                            item.resetShortShares();
                            item.resetShortPrice();
                        } else {
                            inc = (item.getShortPrice() - price) * buyShares; // (OP - NP) * NS

                            // some short positions are "consumed" and part of withheld money is returned
                            double ret = item.getBorrowedBalance() * (double(buyShares) / double(item.getShortShares()));
                            ret = std::floor(ret * std::pow(10, 2)) / std::pow(10, 2);
                            m_porfolioSummary.returnBalance(ret);
                            item.addBorrowedBalance(-ret);

                            item.addShortShares(-buyShares);
                            if (item.getShortShares() == 0) {
                                item.resetShortPrice();
                            }
                        }
                    }

                    // the user must pay for the share that were bought
                    m_porfolioSummary.addBuyingPower(-buyShares * price);
                    // but pending transaction price is returned (also updating total holding balance)
                    m_porfolioSummary.releaseBalance(m_pendingBidOrders[reportPtr->orderID].getPrice() * buyShares);

                    m_pendingBidOrders[reportPtr->orderID].setSize(m_pendingBidOrders[reportPtr->orderID].getSize() - (buyShares / 100));
                    reportPtr->currentSize = m_pendingBidOrders[reportPtr->orderID].getSize();

                    if (m_pendingBidOrders[reportPtr->orderID].getSize() == 0) { // if pending transaction is completely fulfilled
                        m_pendingBidOrders.erase(reportPtr->orderID); // it can be deleted
                    } else {
                        reportPtr->orderStatus = Order::Status::PARTIALLY_FILLED;
                    }

                    item.addPL(inc); // update instrument P&L
                    m_porfolioSummary.addTotalPL(inc); // update portfolio P&L
                    m_porfolioSummary.addTotalShares(buyShares); // update total trade shares

                } else if (reportPtr->orderType == Order::Type::MARKET_SELL || reportPtr->orderType == Order::Type::LIMIT_SELL) {

                    std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                    std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
                    std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

                    auto& item = m_portfolioItems[reportPtr->orderSymbol];

                    const double price = reportPtr->orderPrice; // NP
                    const int sellShares = reportPtr->executedSize * 100; // NS

                    double inc = 0.0;

                    if (m_pendingAskOrders[reportPtr->orderID].second == 0) { // no long shares were reserved for this order
                        m_porfolioSummary.borrowBalance(price * sellShares); // all shares need to be borrowed
                        item.addBorrowedBalance(price * sellShares);
                        m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * sellShares;

                        item.addShortPrice(price, sellShares);
                        item.addShortShares(sellShares);
                    } else {
                        if (sellShares < m_pendingAskOrders[reportPtr->orderID].second) { // if enough shares were previously reserved
                            inc = (price - item.getLongPrice()) * sellShares; // (NP - OP) * NS

                            // update buying power for selling shares
                            m_porfolioSummary.addBuyingPower(sellShares * price);

                            item.addLongShares(-sellShares);
                            if (item.getLongShares() == 0) {
                                item.resetLongPrice();
                            }

                            m_pendingShortUnitAmount[reportPtr->orderSymbol] -= sellShares;
                            m_pendingAskOrders[reportPtr->orderID].second -= sellShares; // update reservation of shares of this transaction
                        } else {
                            inc = (price - item.getLongPrice()) * m_pendingAskOrders[reportPtr->orderID].second; // (NP - OP) * OS

                            // update buying power for selling long shares
                            m_porfolioSummary.addBuyingPower(m_pendingAskOrders[reportPtr->orderID].second * price);

                            double rem = sellShares - m_pendingAskOrders[reportPtr->orderID].second;
                            m_porfolioSummary.borrowBalance(price * rem); // the remainder of the shares need to be borrowed
                            item.addBorrowedBalance(price * rem);
                            m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * rem;

                            item.addShortPrice(price, rem);
                            item.addShortShares(rem);
                            item.addLongShares(-m_pendingAskOrders[reportPtr->orderID].second);
                            if (item.getLongShares() == 0) {
                                item.resetLongPrice();
                            }

                            m_pendingShortUnitAmount[reportPtr->orderSymbol] -= m_pendingAskOrders[reportPtr->orderID].second;
                            m_pendingAskOrders[reportPtr->orderID].second = 0; // all reserved shares of this transactions were already used
                        }
                    }

                    m_pendingAskOrders[reportPtr->orderID].first.setSize(m_pendingAskOrders[reportPtr->orderID].first.getSize() - (sellShares / 100));
                    reportPtr->currentSize = m_pendingAskOrders[reportPtr->orderID].first.getSize();

                    if (m_pendingAskOrders[reportPtr->orderID].first.getSize() == 0) { // if pending transaction is completely fulfilled
                        m_pendingAskOrders.erase(reportPtr->orderID); // it can be deleted
                    } else {
                        reportPtr->orderStatus = Order::Status::PARTIALLY_FILLED;
                    }

                    item.addPL(inc); // update instrument P&L
                    m_porfolioSummary.addTotalPL(inc); // update portfolio P&L
                    m_porfolioSummary.addTotalShares(sellShares); // update total trade shares
                }

            } else if (reportPtr->orderStatus == Order::Status::CANCELED) { // cancellation report

                std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

                const int cancelShares = reportPtr->executedSize * 100;

                if (reportPtr->orderType == Order::Type::CANCEL_BID) {
                    // pending transaction price is returned
                    m_porfolioSummary.releaseBalance(m_pendingBidOrders[reportPtr->orderID].getPrice() * cancelShares);

                    m_pendingBidOrders[reportPtr->orderID].setSize(m_pendingBidOrders[reportPtr->orderID].getSize() - (cancelShares / 100));

                    if (m_pendingBidOrders[reportPtr->orderID].getSize() == 0) { // if pending transaction is completely cancelled
                        m_pendingBidOrders.erase(reportPtr->orderID); // it can be deleted
                    }
                } else if (reportPtr->orderType == Order::Type::CANCEL_ASK) {
                    const int shortShares = m_pendingAskOrders[reportPtr->orderID].first.getSize() * 100 - m_pendingAskOrders[reportPtr->orderID].second;

                    if (cancelShares < shortShares) {
                        m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * cancelShares;
                    } else {
                        m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * shortShares;
                        m_pendingShortUnitAmount[reportPtr->orderSymbol] -= (cancelShares - shortShares);
                        m_pendingAskOrders[reportPtr->orderID].second -= (cancelShares - shortShares);
                    }

                    m_pendingAskOrders[reportPtr->orderID].first.setSize(m_pendingAskOrders[reportPtr->orderID].first.getSize() - (cancelShares / 100));

                    if (m_pendingAskOrders[reportPtr->orderID].first.getSize() == 0) { // if pending transaction is completely cancelled
                        m_pendingAskOrders.erase(reportPtr->orderID); // it can be deleted
                    }
                }
            }

            bool wasPortfolioSent = false; // to postpone DB writes until lock guards are released
            {
                std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);

                s_sendPortfolioSummaryToUser(m_userID, m_porfolioSummary);
                s_sendPortfolioItemToUser(m_userID, m_portfolioItems[reportPtr->orderSymbol]);

                wasPortfolioSent = true;
            }
            if (!DBConnector::s_isPortfolioDBReadOnly && wasPortfolioSent) {
                const auto& item = m_portfolioItems[reportPtr->orderSymbol];
                auto lock { DBConnector::getInstance()->lockPSQL() };

                DBConnector::getInstance()->doQuery(
                    "UPDATE portfolio_items" // presume that we have got the user's uuid already in it
                    "\n"
                    "SET borrowed_balance = "
                        + std::to_string(item.getBorrowedBalance())
                        + ", pl = " + std::to_string(item.getPL())
                        + ", long_price = " + std::to_string(item.getLongPrice())
                        + ", short_price = " + std::to_string(item.getShortPrice())
                        + ", long_shares = " + std::to_string(item.getLongShares())
                        + ", short_shares = " + std::to_string(item.getShortShares())
                        + "\n" // PK == (id, symbol):
                          "WHERE symbol = '"
                        + reportPtr->orderSymbol + "' AND id = '"
                        + m_userID + "';",
                    COLOR_WARNING "WARNING: UPDATE portfolio_items failed for user [" + m_userID + "]!\n" NO_COLOR);

                DBConnector::getInstance()->doQuery(
                    "UPDATE portfolio_summary" // presume that we have got the user's uuid already in it
                    "\n"
                    "SET buying_power = "
                        + std::to_string(m_porfolioSummary.getBuyingPower())
                        + ", holding_balance = " + std::to_string(m_porfolioSummary.getHoldingBalance())
                        + ", borrowed_balance = " + std::to_string(m_porfolioSummary.getBorrowedBalance())
                        + ", total_pl = " + std::to_string(m_porfolioSummary.getTotalPL())
                        + ", total_shares = " + std::to_string(m_porfolioSummary.getTotalShares())
                        + "\n" // PK == id:
                          "WHERE id = '"
                        + m_userID + "';",
                    COLOR_WARNING "WARNING: UPDATE portfolio_summary failed for user [" + m_userID + "]!\n" NO_COLOR);
            }
        }

        updateWaitingList(*reportPtr);
        sendWaitingList();
        FIXAcceptor::getInstance()->sendConfirmationReport(*reportPtr);

        reportPtr = nullptr;
        m_execRptBuffer.pop();
    } // while
}

bool RiskManagement::verifyAndSendOrder(const Order& order)
{
    bool success = false;
    double price = order.getPrice();

    // the usable buying power is the buying power minus the cash amount reserved for pending short orders
    double usableBuyingPower = 0.0;
    {
        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);
        usableBuyingPower = m_porfolioSummary.getBuyingPower() - m_pendingShortCashAmount;
    }

    switch (order.getType()) {
    case Order::Type::MARKET_BUY: {
        price = getMarketSellPrice(order.getSymbol()); // use market price
    } // the rest is the same as in limit orders
    case Order::Type::LIMIT_BUY: {
        if (price == 0.0)
            break;

        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

        if (price * order.getSize() * 100 < usableBuyingPower) {
            m_porfolioSummary.holdBalance(price * order.getSize() * 100);

            m_pendingBidOrders[order.getID()] = order; // store the pending transaction
            m_pendingBidOrders[order.getID()].setPrice(price); // update the price of the saved pending transaction (necessary for market orders)

            success = true;
        }
    } break;
    case Order::Type::MARKET_SELL: {
        price = getMarketBuyPrice(order.getSymbol()); // use market price
    } // the rest is the same as in limit orders
    case Order::Type::LIMIT_SELL: {
        if (price == 0.0)
            break;

        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
        std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

        // the number of available shares is the total of long shares minus the share amount reserved for pending sell orders
        int availableShares = m_portfolioItems[order.getSymbol()].getLongShares() - m_pendingShortUnitAmount[order.getSymbol()];
        int shortShares = order.getSize() * 100 - availableShares; // this is the amount of shares that need to be shorted

        if (shortShares <= 0) { // no short positions are necessary
            m_pendingShortUnitAmount[order.getSymbol()] += order.getSize() * 100; // reserve all shares of this order
            m_pendingAskOrders[order.getID()] = std::make_pair(order, order.getSize() * 100); // store the pending transaction along with reserved shares
            m_pendingAskOrders[order.getID()].first.setPrice(price);

            success = true;
        } else if ((m_porfolioSummary.getBorrowedBalance() < usableBuyingPower) && (price * shortShares < usableBuyingPower)) {
            // m_porfolioSummary.getBorrowedBalance() < usableBuyingPower means it is still possible to "recover" from all short positions
            // i.e. the user still has enough money to buy everything back
            m_pendingShortCashAmount += price * shortShares; // reserve the necessary cash amount for this order
            m_pendingShortUnitAmount[order.getSymbol()] += availableShares; // reserve remaining shares for this order
            m_pendingAskOrders[order.getID()] = std::make_pair(order, availableShares); // store the pending transaction along with reserved shares
            m_pendingAskOrders[order.getID()].first.setPrice(price);

            success = true;
        }
    } break;
    case Order::Type::CANCEL_BID: {
        std::lock_guard<std::mutex> guard(m_mtxWaitingList);
        auto it = m_waitingList.find(order.getID());
        if (m_waitingList.end() != it) // found order id
            if ((it->second.getType() == Order::Type::LIMIT_BUY) || (it->second.getType() == Order::Type::MARKET_BUY))
                if (it->second.getPrice() == order.getPrice())
                    success = true;
    } break;
    case Order::Type::CANCEL_ASK: {
        std::lock_guard<std::mutex> guard(m_mtxWaitingList);
        auto it = m_waitingList.find(order.getID());
        if (m_waitingList.end() != it) // found order id
            if ((it->second.getType() == Order::Type::LIMIT_SELL) || (it->second.getType() == Order::Type::MARKET_SELL))
                if (it->second.getPrice() == order.getPrice())
                    success = true;
    } break;
    default: {
    } break;
    }

    if (success) {
        s_sendOrderToME(order);
    } else {
        ExecutionReport report {
            m_userID,
            order.getID(),
            order.getType(),
            order.getSymbol(),
            order.getSize(),
            0, // executed size
            order.getPrice(),
            Order::Status::REJECTED,
            "BC" // destination (rejected at the Brokerage Center)
        };
        FIXAcceptor::getInstance()->sendConfirmationReport(report);
    }

    return success;
}
