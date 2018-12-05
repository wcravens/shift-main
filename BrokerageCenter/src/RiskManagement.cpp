#include "RiskManagement.h"

#include "BCDocuments.h"
#include "FIXAcceptor.h"
#include "FIXInitiator.h"

// #include "DBConnector.h"

#include <cmath>

#include <shift/miscutils/concurrency/Consumer.h>
// #include <shift/miscutils/terminal/Common.h>

// static inline std::string s_getPortfolioItemsCursorName(const std::string& userName)
// {
//     return "csr_pi_" + userName;
// }

// static inline std::string s_getPortfolioSummaryCursorName(const std::string& userName)
// {
//     return "csr_ps_" + userName;
// }

// static void s_declPortfolioItemsCursor(const std::string& userName, const std::string& symbol)
// {
//     if(userName.empty()) return;

//     const auto& csrName = s_getPortfolioItemsCursorName(userName);
//     DBConnector::instance()->doQuery(
//         "DECLARE " + csrName + " CURSOR FOR"
//         "  SELECT * FROM traders INNER JOIN portfolio_items ON traders.id = portfolio_items.client_id\n"
//         "WHERE traders.username = '" + userName + "' AND portfolio_items.symbol = '" + symbol + '\''
//         , COLOR_ERROR "ERROR: DECLARE CURSOR failed. (RiskManagement." + csrName + ")\n" NO_COLOR);
// }

// static void s_declPortfolioSummaryCursor(const std::string& userName)
// {
//     if(userName.empty()) return;

//     const auto& csrName = s_getPortfolioSummaryCursorName(userName);
//     DBConnector::instance()->doQuery(
//         "DECLARE " + csrName + " CURSOR FOR"
//         "  SELECT * FROM traders INNER JOIN portfolio_summary ON traders.id = portfolio_summary.client_id\n"
//         "WHERE traders.username = '" + userName + '\''
//         , COLOR_ERROR "ERROR: DECLARE CURSOR failed. (RiskManagement." + csrName + ")\n" NO_COLOR);
// }

RiskManagement::RiskManagement(std::string userName, double buyingPower)
    : m_userName(std::move(userName))
    , m_porfolioSummary{ buyingPower }
    , m_pendingShortCashAmount{ 0.0 }
{
    // s_declPortfolioSummaryCursor(userName);
}

RiskManagement::RiskManagement(std::string userName, double buyingPower, int totalShares)
    : m_userName(std::move(userName))
    , m_porfolioSummary{ buyingPower, totalShares }
    , m_pendingShortCashAmount{ 0.0 }
{
    // s_declPortfolioSummaryCursor(userName);
}

RiskManagement::RiskManagement(std::string userName, double buyingPower, double holdingBalance, double borrowedBalance, double totalPL, int totalShares)
    : m_userName(std::move(userName))
    , m_porfolioSummary{ buyingPower, holdingBalance, borrowedBalance, totalPL, totalShares }
    , m_pendingShortCashAmount{ 0.0 }
{
    // s_declPortfolioSummaryCursor(userName);
}

RiskManagement::~RiskManagement()
{
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlagExec, m_cvExecRpt, *m_execRptThread);
    m_execRptThread = nullptr;
    shift::concurrency::notifyConsumerThreadToQuit(m_quitFlagQuote, m_cvQuote, *m_quoteThread);
    m_quoteThread = nullptr;
}

void RiskManagement::spawn()
{
    m_quoteThread.reset(new std::thread(&RiskManagement::processQuote, this));
    m_execRptThread.reset(new std::thread(&RiskManagement::processExecRpt, this));
}

void RiskManagement::enqueueQuote(const Quote& quote)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxQuote);
        m_quoteBuffer.push(quote);
    }
    m_cvQuote.notify_one();
}

void RiskManagement::enqueueExecRpt(const Report& report)
{
    {
        std::lock_guard<std::mutex> guard(m_mtxExecRpt);
        m_execRptBuffer.push(report);
    }
    m_cvExecRpt.notify_one();
}

inline void RiskManagement::sendQuoteToME(const Quote& quote)
{
    FIXInitiator::instance()->sendQuote(quote);
}

inline void RiskManagement::sendPortfolioSummaryToClient(const std::string& userName, const PortfolioSummary& summary)
{
    const auto& targetID = BCDocuments::instance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        std::cout << " Don't exist: " << userName << std::endl;
        return;
    }

    FIXAcceptor::instance()->sendPortfolioSummary(userName, targetID, summary);
}

inline void RiskManagement::sendPortfolioItemToClient(const std::string& userName, const PortfolioItem& item)
{
    const auto& targetID = BCDocuments::instance()->getTargetIDByUserName(userName);
    if (CSTR_NULL == targetID) {
        std::cout << " Don't exist: " << userName << std::endl;
        return;
    }

    FIXAcceptor::instance()->sendPortfolioItem(userName, targetID, item);
}

void RiskManagement::sendPortfolioHistory()
{
    std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
    std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);

    sendPortfolioSummaryToClient(m_userName, m_porfolioSummary);

    for (auto& i : m_portfolioItems)
        sendPortfolioItemToClient(m_userName, i.second);
}

void RiskManagement::sendQuoteHistory() const
{
    std::lock_guard<std::mutex> guard(m_mtxQuoteHistory);
    if (!m_quoteHistory.empty())
        FIXAcceptor::instance()->sendQuoteHistory(m_userName, m_quoteHistory);
}

void RiskManagement::updateQuoteHistory(const Report& report)
{
    if (report.status == FIX::OrdStatus_NEW)
        return;

    std::lock_guard<std::mutex> guard(m_mtxQuoteHistory);
    auto it = m_quoteHistory.find(report.orderID);
    if (m_quoteHistory.end() == it)
        return;

    auto& quote = it->second;
    if (quote.getShareSize() > report.shareSize) {
        quote.setShareSize(quote.getShareSize() - report.shareSize);
    } else {
        if (m_quoteHistory.size() == 1) {
            quote.setShareSize(0);
            quote.setPrice(0.0);
        } else {
            m_quoteHistory.erase(it);
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
    return BCDocuments::instance()->getStockOrderBookMarketFirstPrice(true, symbol);
}

inline double RiskManagement::getMarketSellPrice(const std::string& symbol)
{
    return BCDocuments::instance()->getStockOrderBookMarketFirstPrice(false, symbol);
}

void RiskManagement::processQuote()
{
    using QOT = Quote::ORDER_TYPE;

    thread_local auto quitFut = m_quitFlagQuote.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxQuote);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvQuote, lock, [this] { return !m_quoteBuffer.empty(); }))
            return;

        auto quotePtr = &m_quoteBuffer.front();

        if (m_portfolioItems.find(quotePtr->getSymbol()) == m_portfolioItems.end()) { // create new portfolio item
            std::lock_guard<std::mutex> guard(m_mtxPortfolioItems);
            m_portfolioItems.emplace(quotePtr->getSymbol(), quotePtr->getSymbol());
        }

        if (verifyAndSendQuote(*quotePtr)) {
            {
                std::lock_guard<std::mutex> guard(m_mtxQuoteHistory);
                if (quotePtr->getOrderType() != QOT::CANCEL_BID && quotePtr->getOrderType() != QOT::CANCEL_ASK) {
                    m_quoteHistory[quotePtr->getOrderID()] = *quotePtr; // save to m_quoteHistory map
                }
            }
            sendQuoteHistory();
        }

        quotePtr = nullptr;
        m_quoteBuffer.pop();
    }
}

void RiskManagement::processExecRpt()
{
    using QOT = Quote::ORDER_TYPE;

    thread_local auto quitFut = m_quitFlagExec.get_future();

    while (true) {
        std::unique_lock<std::mutex> lock(m_mtxExecRpt);
        if (shift::concurrency::quitOrContinueConsumerThread(quitFut, m_cvExecRpt, lock, [this] { return !m_execRptBuffer.empty(); }))
            return;

        auto reportPtr = &m_execRptBuffer.front();

        switch (reportPtr->status) {
        case FIX::OrdStatus_NEW: { // confirmation report, not used in LC
            FIXAcceptor::instance()->sendConfirmationReport(*reportPtr);
        } break;
        case FIX::ExecType_FILL: { // execution report
            // DBConnector::instance()->doQuery("BEGIN", "");
            // s_declPortfolioItemsCursor(m_userName, reportPtr->symbol);

            switch (reportPtr->orderType) {
            case QOT::MARKET_BUY:
            case QOT::LIMIT_BUY: {
                std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
                std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
                std::lock_guard<std::mutex> qpGuard(m_mtxQuoteProcessing);

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
                        // DBConnector::instance()->doQuery(
                        //     "UPDATE portfolio_items\n"
                        //     "SET portfolio_items.borrowed_balance = " + std::to_string(item.getBorrowedBalance()) + "\n"
                        //     "WHERE CURRENT OF " + s_getPortfolioItemsCursorName(m_userName)
                        //     , ""
                        // );

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
                std::lock_guard<std::mutex> qpGuard(m_mtxQuoteProcessing);

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

            } break;
            }

            // DBConnector::instance()->doQuery("CLOSE " + s_getPortfolioItemsCursorName(m_userName), "");
            // DBConnector::instance()->doQuery("END", "");
        } break;
        case FIX::ExecType_EXPIRED: { // cancellation report
            std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
            std::lock_guard<std::mutex> qpGuard(m_mtxQuoteProcessing);

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
        }

        {
            std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
            std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);

            sendPortfolioItemToClient(m_userName, m_portfolioItems[reportPtr->symbol]);
            sendPortfolioSummaryToClient(m_userName, m_porfolioSummary);
        }

        updateQuoteHistory(*reportPtr);
        sendQuoteHistory();

        reportPtr = nullptr;
        m_execRptBuffer.pop();
    }
}

bool RiskManagement::verifyAndSendQuote(const Quote& quote)
{
    using QOT = Quote::ORDER_TYPE;

    bool success = false;
    double price = quote.getPrice();

    // the usable buying power is the buying power minus the cash amount reserved for pending short orders
    double usableBuyingPower = 0.0;
    {
        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> qpGuard(m_mtxQuoteProcessing);
        usableBuyingPower = m_porfolioSummary.getBuyingPower() - m_pendingShortCashAmount;
    }

    switch (quote.getOrderType()) {
    case QOT::MARKET_BUY: {
        price = getMarketSellPrice(quote.getSymbol()); // use market price
        if (price == 0.0)
            break;
    } // the rest is the same as in limit orders
    case QOT::LIMIT_BUY: {
        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> qpGuard(m_mtxQuoteProcessing);

        if (price * quote.getShareSize() * 100 < usableBuyingPower) {
            m_porfolioSummary.holdBalance(price * quote.getShareSize() * 100);

            m_pendingBidOrders[quote.getOrderID()] = quote; // store the pending transaction
            m_pendingBidOrders[quote.getOrderID()].setPrice(price); // update the price of the saved pending transaction (necessary for market orders)

            success = true;
        }
    } break;
    case QOT::MARKET_SELL: {
        price = getMarketBuyPrice(quote.getSymbol()); // use market price
        if (price == 0.0)
            break;
    } // the rest is the same as in limit orders
    case QOT::LIMIT_SELL: {
        std::lock_guard<std::mutex> psGuard(m_mtxPortfolioSummary);
        std::lock_guard<std::mutex> piGuard(m_mtxPortfolioItems);
        std::lock_guard<std::mutex> qpGuard(m_mtxQuoteProcessing);

        // the number of available shares is the total of long shares minus the share amount reserved for pending sell orders
        int availableShares = m_portfolioItems[quote.getSymbol()].getLongShares() - m_pendingShortUnitAmount[quote.getSymbol()];
        int shortShares = quote.getShareSize() * 100 - availableShares; // this is the amount of shares that need to be shorted

        if (shortShares <= 0) { // no short positions are necessary

            m_pendingShortUnitAmount[quote.getSymbol()] += quote.getShareSize() * 100; // reserve all shares of this order
            m_pendingAskOrders[quote.getOrderID()] = std::make_pair(quote, quote.getShareSize() * 100); // store the pending transaction along with reserved shares
            m_pendingAskOrders[quote.getOrderID()].first.setPrice(price);

            success = true;

        } else if ((m_porfolioSummary.getBorrowedBalance() < usableBuyingPower) && (price * shortShares < usableBuyingPower)) {
            // m_porfolioSummary.getBorrowedBalance() < usableBuyingPower means it is still possible to "recover" from all short positions
            // i.e. the user still has enough money to buy everything back
            m_pendingShortCashAmount += price * shortShares; // reserve the necessary cash amount for this order
            m_pendingShortUnitAmount[quote.getSymbol()] += availableShares; // reserve remaining shares for this order
            m_pendingAskOrders[quote.getOrderID()] = std::make_pair(quote, availableShares); // store the pending transaction along with reserved shares
            m_pendingAskOrders[quote.getOrderID()].first.setPrice(price);

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
        sendQuoteToME(quote);
    }

    return success;
}
