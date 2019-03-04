#include "RiskManagement.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"
#include "FIXInitiator.h"

#include "DBConnector.h"

#include <cmath>

#include <shift/miscutils/concurrency/Consumer.h>
#include <shift/miscutils/terminal/Common.h>

RiskManagement::RiskManagement(std::string username, double buyingPower)
    : m_username(std::move(username))
    , m_porfolioSummary{ buyingPower }
    , m_pendingShortCashAmount{ 0.0 }
{
}

RiskManagement::RiskManagement(std::string username, double buyingPower, int totalShares)
    : m_username(std::move(username))
    , m_porfolioSummary{ buyingPower, totalShares }
    , m_pendingShortCashAmount{ 0.0 }
{
}

RiskManagement::RiskManagement(std::string username, double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares)
    : m_username(std::move(username))
    , m_porfolioSummary{ buyingPower, holdingBalance, borrowedBalance, totalPL, totalShares }
    , m_pendingShortCashAmount{ 0.0 }
{
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

void RiskManagement::enqueueOrder(const Order& order)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxOrder);
        m_orderBuffer.push(order);
    }
    m_cvOrder.notify_one();
}

void RiskManagement::enqueueExecRpt(const Report& report)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxExecRpt);
        m_execRptBuffer.push(report);
    }
    m_cvExecRpt.notify_one();
}

/*static*/ inline void RiskManagement::s_sendOrderToME(const Order& order)
{
    FIXInitiator::getInstance()->sendOrder(order);
}

/*static*/ inline void RiskManagement::s_sendPortfolioSummaryToClient(const std::string& username, const PortfolioSummary& summary)
{
    FIXAcceptor::getInstance()->sendPortfolioSummary(username, summary);
}

/*static*/ inline void RiskManagement::s_sendPortfolioItemToClient(const std::string& username, const PortfolioItem& item)
{
    FIXAcceptor::getInstance()->sendPortfolioItem(username, item);
}

void RiskManagement::sendPortfolioHistory()
{
    std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
    std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);

    s_sendPortfolioSummaryToClient(m_username, m_porfolioSummary);

    for (auto& i : m_portfolioItems)
        s_sendPortfolioItemToClient(m_username, i.second);
}

void RiskManagement::sendOrderHistory() const
{
    std::lock_guard<std::mutex> guard(m_mtxOrderHistory);
    if (!m_orderHistory.empty())
        FIXAcceptor::getInstance()->sendOrderHistory(m_username, m_orderHistory);
}

void RiskManagement::updateOrderHistory(const Report& report)
{
    if (report.status == FIX::OrdStatus_NEW)
        return;

    std::lock_guard<std::mutex> guard(m_mtxOrderHistory);
    auto it = m_orderHistory.find(report.orderID);
    if (m_orderHistory.end() == it)
        return;

    auto& order = it->second;
    if (order.getShareSize() > report.shareSize) {
        order.setShareSize(order.getShareSize() - report.shareSize);
    } else {
        if (m_orderHistory.size() == 1) {
            order.setShareSize(0);
            order.setPrice(0.0);
        } else {
            m_orderHistory.erase(it);
        }
    }
}

void RiskManagement::insertPortfolioItem(const std::string& symbol, const PortfolioItem& portfolioItem)
{
    std::lock_guard<std::mutex> guard(m_mtxPortfolioItems);
    m_portfolioItems[symbol] = portfolioItem;
}

inline double RiskManagement::getMarketBuyPrice(const std::string& symbol)
{
    return BCDocuments::getInstance()->getStockOrderBookMarketFirstPrice(true, symbol);
}

inline double RiskManagement::getMarketSellPrice(const std::string& symbol)
{
    return BCDocuments::getInstance()->getStockOrderBookMarketFirstPrice(false, symbol);
}

void RiskManagement::processOrder()
{
    using QOT = Order::ORDER_TYPE;

    thread_local auto quitFut = m_quitFlagOrder.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxOrder);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvOrder, lock, [this] { return !m_orderBuffer.empty(); }))
            return;

        auto orderPtr = &m_orderBuffer.front();

        if (m_portfolioItems.find(orderPtr->getSymbol()) == m_portfolioItems.end()) { // add new portfolio item ?
            insertPortfolioItem(orderPtr->getSymbol(), orderPtr->getSymbol());

            if (!DBConnector::s_isPortfolioDBReadOnly) {
                auto lock{ DBConnector::getInstance()->lockPSQL() };
                DBConnector::getInstance()->doQuery("INSERT INTO portfolio_items (id, symbol) VALUES ((SELECT id FROM traders WHERE username = '" + m_username + "'), '" + orderPtr->getSymbol() + "');", "");
            }
        }

        if (verifyAndSendOrder(*orderPtr)) {
            {
                std::lock_guard<std::mutex> guard(m_mtxOrderHistory);
                if (orderPtr->getOrderType() != QOT::CANCEL_BID && orderPtr->getOrderType() != QOT::CANCEL_ASK) {
                    m_orderHistory[orderPtr->getOrderID()] = *orderPtr;
                }
            }

            sendOrderHistory();
        }

        orderPtr = nullptr;
        m_orderBuffer.pop();
    }
}

void RiskManagement::processExecRpt()
{
    using QOT = Order::ORDER_TYPE;

    thread_local auto quitFut = m_quitFlagExec.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxExecRpt);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvExecRpt, lock, [this] { return !m_execRptBuffer.empty(); }))
            return;

        auto reportPtr = &m_execRptBuffer.front();

        switch (reportPtr->status) {
        case FIX::OrdStatus_NEW: { // confirmation report, not used in LC
            FIXAcceptor::getInstance()->sendConfirmationReport(*reportPtr);
        } break;
        case FIX::ExecType_FILL: { // execution report
            switch (reportPtr->orderType) {
            case QOT::MARKET_BUY:
            case QOT::LIMIT_BUY: {
                std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
                std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

                auto& item = m_portfolioItems[reportPtr->symbol];

                const double price = reportPtr->price; // NP
                const int buyShares = reportPtr->shareSize * 100; // NS

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

                m_pendingBidOrders[reportPtr->orderID].setShareSize(m_pendingBidOrders[reportPtr->orderID].getShareSize() - (buyShares / 100));

                if (m_pendingBidOrders[reportPtr->orderID].getShareSize() == 0) { // if pending transaction is completely fulfilled
                    m_pendingBidOrders.erase(reportPtr->orderID); // it can be deleted
                }

                item.addPL(inc); // update instrument P&L
                m_porfolioSummary.addTotalPL(inc); // update portfolio P&L
                m_porfolioSummary.addTotalShares(buyShares); // update total trade shares
            } break;

            case QOT::MARKET_SELL:
            case QOT::LIMIT_SELL: {
                std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
                std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

                auto& item = m_portfolioItems[reportPtr->symbol];

                const double price = reportPtr->price; // NP
                const int sellShares = reportPtr->shareSize * 100; // NS

                double inc = 0.0;

                if (m_pendingAskOrders[reportPtr->orderID].second == 0) { // no long shares were reserved for this order
                    m_porfolioSummary.borrowBalance(price * sellShares); // all shares need to be borrowed
                    item.addBorrowedBalance(price * sellShares);
                    m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * sellShares;

                    item.addShortPrice(price, sellShares);
                    item.addShortShares(sellShares);
                } else {
                    if (sellShares < m_pendingAskOrders[reportPtr->orderID].second) { // if enought shares were previously reserved
                        inc = (price - item.getLongPrice()) * sellShares; // (NP - OP) * NS

                        // update buying power for selling shares
                        m_porfolioSummary.addBuyingPower(sellShares * price);

                        item.addLongShares(-sellShares);
                        if (item.getLongShares() == 0) {
                            item.resetLongPrice();
                        }

                        m_pendingShortUnitAmount[reportPtr->symbol] -= sellShares;
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

                        m_pendingShortUnitAmount[reportPtr->symbol] -= m_pendingAskOrders[reportPtr->orderID].second;
                        m_pendingAskOrders[reportPtr->orderID].second = 0; // all reserved shares of this transactions were already used
                    }
                }

                m_pendingAskOrders[reportPtr->orderID].first.setShareSize(m_pendingAskOrders[reportPtr->orderID].first.getShareSize() - (sellShares / 100));

                if (m_pendingAskOrders[reportPtr->orderID].first.getShareSize() == 0) { // if pending transaction is completely fulfilled
                    m_pendingAskOrders.erase(reportPtr->orderID); // it can be deleted
                }

                item.addPL(inc); // update instrument P&L
                m_porfolioSummary.addTotalPL(inc); // update portfolio P&L
                m_porfolioSummary.addTotalShares(sellShares); // update total trade shares
            } break; // case
            } // switch
        } break;

        case FIX::ExecType_EXPIRED: { // cancellation report
            std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
            std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

            const int cancelShares = reportPtr->shareSize * 100;

            if (reportPtr->orderType == QOT::CANCEL_BID) {
                // pending transaction price is returned
                m_porfolioSummary.releaseBalance(m_pendingBidOrders[reportPtr->orderID].getPrice() * cancelShares);

                m_pendingBidOrders[reportPtr->orderID].setShareSize(m_pendingBidOrders[reportPtr->orderID].getShareSize() - (cancelShares / 100));

                if (m_pendingBidOrders[reportPtr->orderID].getShareSize() == 0) { // if pending transaction is completely cancelled
                    m_pendingBidOrders.erase(reportPtr->orderID); // it can be deleted
                }
            } else if (reportPtr->orderType == QOT::CANCEL_ASK) {
                const int shortShares = m_pendingAskOrders[reportPtr->orderID].first.getShareSize() * 100 - m_pendingAskOrders[reportPtr->orderID].second;

                if (cancelShares < shortShares) {
                    m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * cancelShares;
                } else {
                    m_pendingShortCashAmount -= m_pendingAskOrders[reportPtr->orderID].first.getPrice() * shortShares;
                    m_pendingShortUnitAmount[reportPtr->symbol] -= (cancelShares - shortShares);
                    m_pendingAskOrders[reportPtr->orderID].second -= (cancelShares - shortShares);
                }

                m_pendingAskOrders[reportPtr->orderID].first.setShareSize(m_pendingAskOrders[reportPtr->orderID].first.getShareSize() - (cancelShares / 100));

                if (m_pendingAskOrders[reportPtr->orderID].first.getShareSize() == 0) { // if pending transaction is completely cancelled
                    m_pendingAskOrders.erase(reportPtr->orderID); // it can be deleted
                }
            }
        } break;
        } // switch

        bool wasPortfolioSent = false; // to postpone DB writes until lock guards are released
        {
            std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
            std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);

            s_sendPortfolioSummaryToClient(m_username, m_porfolioSummary);
            s_sendPortfolioItemToClient(m_username, m_portfolioItems[reportPtr->symbol]);

            wasPortfolioSent = true;
        }
        if (!DBConnector::s_isPortfolioDBReadOnly && wasPortfolioSent) {
            const auto& item = m_portfolioItems[reportPtr->symbol];
            auto lock{ DBConnector::getInstance()->lockPSQL() };

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
                    + reportPtr->symbol + "' AND id = (SELECT id FROM traders WHERE username = '"
                    + m_username + "');",
                COLOR_WARNING "WARNING: UPDATE portfolio_items failed for user [" + m_username + "]!\n" NO_COLOR);

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
                      "WHERE id = (SELECT id FROM traders WHERE username = '"
                    + m_username + "');",
                COLOR_WARNING "WARNING: UPDATE portfolio_summary failed for user [" + m_username + "]!\n" NO_COLOR);
        }

        updateOrderHistory(*reportPtr);
        sendOrderHistory();

        reportPtr = nullptr;
        m_execRptBuffer.pop();
    } // while
}

bool RiskManagement::verifyAndSendOrder(const Order& order)
{
    using QOT = Order::ORDER_TYPE;

    bool success = false;
    double price = order.getPrice();

    // the usable buying power is the buying power minus the cash amount reserved for pending short orders
    double usableBuyingPower = 0.0;
    {
        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);
        usableBuyingPower = m_porfolioSummary.getBuyingPower() - m_pendingShortCashAmount;
    }

    switch (order.getOrderType()) {
    case QOT::MARKET_BUY: {
        price = getMarketSellPrice(order.getSymbol()); // use market price
    } // the rest is the same as in limit orders
    case QOT::LIMIT_BUY: {
        if (price == 0.0)
            break;

        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

        if (price * order.getShareSize() * 100 < usableBuyingPower) {
            m_porfolioSummary.holdBalance(price * order.getShareSize() * 100);

            m_pendingBidOrders[order.getOrderID()] = order; // store the pending transaction
            m_pendingBidOrders[order.getOrderID()].setPrice(price); // update the price of the saved pending transaction (necessary for market orders)

            success = true;
        }
    } break;
    case QOT::MARKET_SELL: {
        price = getMarketBuyPrice(order.getSymbol()); // use market price
    } // the rest is the same as in limit orders
    case QOT::LIMIT_SELL: {
        if (price == 0.0)
            break;

        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
        std::lock_guard<std::mutex> qpGuard(m_mtxOrderProcessing);

        // the number of available shares is the total of long shares minus the share amount reserved for pending sell orders
        int availableShares = m_portfolioItems[order.getSymbol()].getLongShares() - m_pendingShortUnitAmount[order.getSymbol()];
        int shortShares = order.getShareSize() * 100 - availableShares; // this is the amount of shares that need to be shorted

        if (shortShares <= 0) { // no short positions are necessary
            m_pendingShortUnitAmount[order.getSymbol()] += order.getShareSize() * 100; // reserve all shares of this order
            m_pendingAskOrders[order.getOrderID()] = std::make_pair(order, order.getShareSize() * 100); // store the pending transaction along with reserved shares
            m_pendingAskOrders[order.getOrderID()].first.setPrice(price);

            success = true;
        } else if ((m_porfolioSummary.getBorrowedBalance() < usableBuyingPower) && (price * shortShares < usableBuyingPower)) {
            // m_porfolioSummary.getBorrowedBalance() < usableBuyingPower means it is still possible to "recover" from all short positions
            // i.e. the user still has enough money to buy everything back
            m_pendingShortCashAmount += price * shortShares; // reserve the necessary cash amount for this order
            m_pendingShortUnitAmount[order.getSymbol()] += availableShares; // reserve remaining shares for this order
            m_pendingAskOrders[order.getOrderID()] = std::make_pair(order, availableShares); // store the pending transaction along with reserved shares
            m_pendingAskOrders[order.getOrderID()].first.setPrice(price);

            success = true;
        }
    } break;
    case QOT::CANCEL_BID: {
        success = true;
    } break;
    case QOT::CANCEL_ASK: {
        success = true;
    } break;
    default: {
    } break;
    }

    if (success) {
        s_sendOrderToME(order);
    }

    return success;
}
