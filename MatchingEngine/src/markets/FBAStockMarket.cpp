#include "markets/FBAStockMarket.h"

#include "markets/MarketCreator.h"

#include "Parameters.h"
#include "TimeSetting.h"

#include <chrono>
#include <thread>

#include <shift/miscutils/terminal/Common.h>

using namespace std::chrono_literals;

namespace markets {

static MarketCreator<FBAStockMarket> creator("FBAStockMarket");

FBAStockMarket::FBAStockMarket(std::string symbol, double batchFrequencyS)
    : Market { std::move(symbol) }
    , m_lastExecutionPrice { 0.0 }
{
    m_batchFrequencyMS = static_cast<long>(batchFrequencyS * 1'000.0); // seconds to milliseconds
    m_nextBatchAuctionMS = m_batchFrequencyMS;
}

FBAStockMarket::FBAStockMarket(const market_creator_parameters_t& parameters)
    : FBAStockMarket { std::get<std::string>(parameters[0]), std::get<double>(parameters[1]) }
{
}

auto FBAStockMarket::getBatchFrequency() const -> double
{
    return static_cast<double>(m_batchFrequencyMS) / 1'000.0; // milliseconds to seconds
}

void FBAStockMarket::setBatchFrequency(double batchFrequencyS)
{
    m_batchFrequencyMS = static_cast<long>(batchFrequencyS * 1'000.0); // seconds to milliseconds
}

auto FBAStockMarket::getNextBatchAuction() const -> double
{
    return static_cast<double>(m_nextBatchAuctionMS) / 1'000.0; // milliseconds to seconds
}

/**
 * @brief Function to start one FBAStockMarket matching engine, for exchange thread.
 */
/* virtual */ void FBAStockMarket::operator()() // override
{
    Order nextOrder {};

    while (!MarketList::s_isTimeout) { // process orders

        // FBA: Batch Auction Stage
        if (auto simulationMS = TimeSetting::getInstance().pastMilli(true); simulationMS >= m_nextBatchAuctionMS) {
            m_nextBatchAuctionMS += m_batchFrequencyMS;
            cout << "FBA: " << simulationMS << "ms" << endl;

            // spinlock implementation:
            // it is better than a standard lock in this scenario, since,
            // most of the time, only this thread needs access to order book data
            while (m_spinlock.test_and_set()) {
            }

            // for debugging:
            // displayGlobalOrderBooks();
            // displayLocalOrderBooks();

            doBatchAuction();
            doIncrementAuctionCounters();

            // for debugging:
            // displayGlobalOrderBooks();
            // displayLocalOrderBooks();

            sendExecutionReports();

            m_spinlock.clear();

            sendOrderBookData("", false, ::FBA_ORDER_BOOK_MAX_LEVEL); // uses m_spinlock

            continue;
        }

        // FBA: Order Submission Stage
        if (!getNextOrder(nextOrder)) {
            std::this_thread::sleep_for(1ms);
            continue;
        }

        // spinlock implementation:
        // it is better than a standard lock in this scenario, since,
        // most of the time, only this thread needs access to order book data
        while (m_spinlock.test_and_set()) {
        }

        switch (nextOrder.getType()) {

        case Order::Type::LIMIT_BUY: {
            insertLocalBid(nextOrder);
            // cout << "Insert Bid" << endl;
            break;
        }

        case Order::Type::LIMIT_SELL: {
            insertLocalAsk(nextOrder);
            // cout << "Insert Ask" << endl;
            break;
        }

        case Order::Type::MARKET_BUY: {
            insertLocalBid(nextOrder);
            // cout << "Insert Bid" << endl;
            break;
        }

        case Order::Type::MARKET_SELL: {
            insertLocalAsk(nextOrder);
            // cout << "Insert Ask" << endl;
            break;
        }

        case Order::Type::CANCEL_BID: {
            doLocalCancelBid(nextOrder);
            // std::cout << "Cancel Bid Done" << endl;
            break;
        }

        case Order::Type::CANCEL_ASK: {
            doLocalCancelAsk(nextOrder);
            // std::cout << "Cancel Ask Done" << endl;
            break;
        }

        case Order::Type::TRTH_TRADE: {
            break;
        }

        case Order::Type::TRTH_BID: {
            break;
        }

        case Order::Type::TRTH_ASK: {
            break;
        }
        }

        // for debugging:
        // displayGlobalOrderBooks();
        // displayLocalOrderBooks();

        // FBA: required for cancellation orders
        sendExecutionReports();

        // FBA: changes to the order book during the order submission stage should not be broadcasted
        // sendOrderBookUpdates();

        m_spinlock.clear();
    }
}

/* static */ auto FBAStockMarket::s_determineExecutionSizes(int totalExecutionSize, const PriceLevel& currentLevel) -> std::deque<int>
{
    std::deque<int> sizes(currentLevel.getNumOrders());

    if (totalExecutionSize >= currentLevel.getSize()) {
        // if there is not enough (or just enough) liquidity in the current level, simply use it all
        int i = 0;
        for (const auto& order : currentLevel) {
            sizes[i] = order.getSize();
            ++i;
        }
    } else {
        // otherwise, divide equally among all orders in the current level
        // --
        // there is a catch: orders that remainined in the book from previous
        // auctions (indicated by their auction counter) should have priority
        int auctionCounter = currentLevel.begin()->getAuctionCounter();
        bool changeCounter = true;

        sizes.assign(sizes.size(), 0);
        int j = totalExecutionSize;
        while (j > 0) {
            int i = 0;
            for (const auto& order : currentLevel) {
                if ((order.getAuctionCounter() == auctionCounter) && (order.getSize() > sizes[i])) {
                    sizes[i] += 1;
                    changeCounter = false;
                    if (--j == 0) {
                        break;
                    }
                } else if (order.getAuctionCounter() < auctionCounter) {
                    // orders are ordered by their auction counter
                    break;
                }
                ++i;
            }
            if (changeCounter) {
                --auctionCounter;
            } else {
                changeCounter = true;
            }
        }

        // for debugging:
        // cout << endl;
        // cout << "Remaining Execution Size: " << totalExecutionSize << endl;
        // cout << endl;
        // cout << "Type\t\tPrice\tSize\tAuction\tExecSize" << endl;
        // int i = 0;
        // for (const auto& order : currentLevel) {
        //     cout << order.getTypeString() << '\t' << order.getPrice() << '\t' << order.getSize() << '\t' << order.getAuctionCounter() << '\t' << sizes[i] << endl;
        //     ++i;
        // }
        // cout << endl;
    }

    return sizes;
}

void FBAStockMarket::doBatchAuction()
{
    auto thisBidLevel = m_localBids.begin();
    auto thisAskLevel = m_localAsks.begin();

    // we must skip market orders to determine if matches are possible
    if ((thisBidLevel != m_localBids.end()) && (thisBidLevel->begin()->getType() == Order::Type::MARKET_BUY)) {
        ++thisBidLevel;
    }
    if ((thisAskLevel != m_localAsks.end()) && (thisAskLevel->begin()->getType() == Order::Type::MARKET_SELL)) {
        ++thisAskLevel;
    }

    if ((thisBidLevel != m_localBids.end()) && (thisAskLevel != m_localAsks.end())) { // none of the order books are empty (or only contain market orders)
        if (thisBidLevel->getPrice() >= thisAskLevel->getPrice()) { // matches are possible
            // -----------------------------------------------------------------
            // 1st step: determine best execution price and size
            // -----------------------------------------------------------------

            /*
             * Execution price selection example 1:
             * - $99.99 selected with the 1st rule: maximum matched orders (519).
             * 
             *      Price | Bid | Demand | Ask | Supply | Excess
             *    $100.09 |   0 |      0 |  54 |   1263 |  -1263
             *    $100.08 | 136 |    136 |  65 |   1209 |  -1073
             *    $100.07 |  50 |    186 | 117 |   1144 |   -958
             *    $100.06 |  26 |    212 | 125 |   1027 |   -815
             *    $100.05 |  28 |    240 |  51 |    902 |   -662
             *    $100.04 |  45 |    285 |  49 |    851 |   -566
             *    $100.03 | 120 |    405 | 124 |    802 |   -397
             *    $100.02 |   0 |    405 |  30 |    678 |   -273
             *    $100.01 |   0 |    405 | 129 |    648 |   -243 
             *    $100.00 |  85 |    490 |   0 |    519 |    -29
             * <<  $99.99 | 163 |    653 | 159 |    519 |    134 >>
             *     $99.98 | 111 |    764 |   0 |    360 |    404
             *     $99.97 |  62 |    826 |  75 |    360 |    466
             *     $99.96 |  23 |    849 |  23 |    285 |    564
             *     $99.95 | 111 |    960 |  40 |    262 |    698
             *     $99.94 | 148 |   1108 | 105 |    222 |    886
             *     $99.93 | 201 |   1309 |  20 |    117 |   1192
             *     $99.92 |  26 |   1335 |  17 |     97 |   1238
             *     $99.91 |  29 |   1364 |  80 |     80 |   1284
            */

            /*
             * Execution price selection example 2:
             * - 2 prices ($99.99 and $100.00) can be selected with the 1st rule.
             * - $100.00 selected with the 2nd rule: minimum absolute excess (29).
             * 
             *      Price | Bid | Demand | Ask | Supply | Excess
             *    $100.09 |  58 |     58 |  54 |   1263 |  -1205
             *    $100.08 | 136 |    194 |  65 |   1209 |  -1015
             *    $100.07 |  50 |    244 | 117 |   1144 |   -900
             *    $100.06 |  26 |    270 | 125 |   1027 |   -757
             *    $100.05 |  28 |    298 |  51 |    902 |   -604
             *    $100.04 |  45 |    343 |  49 |    851 |   -508
             *    $100.03 | 120 |    463 | 124 |    802 |   -339
             *    $100.02 |   0 |    463 |  30 |    678 |   -215
             *    $100.01 |   0 |    463 | 129 |    648 |   -185 
             * << $100.00 |  85 |    548 |   0 |    519 |     29 >>
             *     $99.99 | 163 |    711 | 159 |    519 |    192
             *     $99.98 | 111 |    822 |   0 |    360 |    462
             *     $99.97 |  62 |    884 |  75 |    360 |    524
             *     $99.96 |  23 |    907 |  23 |    285 |    622
             *     $99.95 | 111 |   1018 |  40 |    262 |    756
             *     $99.94 | 148 |   1166 | 105 |    222 |    944
             *     $99.93 | 201 |   1367 |  20 |    117 |   1250
             *     $99.92 |  26 |   1393 |  17 |     97 |   1296
             *     $99.91 |  29 |   1422 |  80 |     80 |   1342
            */

            /*
             * Execution price selection example 3:
             * - 2 prices ($99.99 and $100.00) can be selected with the 1st and 2nd rules.
             * - The 3rd rule should be used: select price closest to last execution price.
             * 
             *      Price | Bid | Demand | Ask | Supply | Excess
             *    $100.09 |  58 |     58 |  54 |   1263 |  -1205
             *    $100.08 | 136 |    194 |  65 |   1209 |  -1015
             *    $100.07 |  50 |    244 | 117 |   1144 |   -900
             *    $100.06 |  26 |    270 | 125 |   1027 |   -757
             *    $100.05 |  28 |    298 |  51 |    902 |   -604
             *    $100.04 |  45 |    343 |  49 |    851 |   -508
             *    $100.03 | 120 |    463 | 124 |    802 |   -339
             *    $100.02 |   0 |    463 |  30 |    678 |   -215
             *    $100.01 |   0 |    463 | 129 |    648 |   -185 
             * << $100.00 |  85 |    548 |   0 |    519 |     29 >>
             * <<  $99.99 |   0 |    548 | 159 |    519 |     29 >>
             *     $99.98 | 111 |    659 |   0 |    360 |    299
             *     $99.97 |  62 |    721 |  75 |    360 |    361
             *     $99.96 |  23 |    744 |  23 |    285 |    459
             *     $99.95 | 111 |    855 |  40 |    262 |    593
             *     $99.94 | 148 |   1003 | 105 |    222 |    781
             *     $99.93 | 201 |   1204 |  20 |    117 |   1087
             *     $99.92 |  26 |   1230 |  17 |     97 |   1133
             *     $99.91 |  29 |   1259 |  80 |     80 |   1179
            */

            int totalDemand = 0;
            int totalSupply = 0;
            std::map<double, std::pair<int, int>> demandSuppyTable;
            int prevValue = 0;
            int minValue = 0;
            int absExcess = 0;
            int minExcess = 0;

            int totalExecutionSize = -1;
            double executionPrice = 0.0;

            // calculate total "general" demand/supply and total demand/supply at individual price levels.
            // the total demand/supply at an individual price level is that price level's demand/supply
            // + the total "general" demand/supply up until that individual price level
            // --
            // both m_localBids and m_localAsk are already ordered by best prices for that
            // particular order book type (bid or ask), so we can simply iterate in order:
            for (const auto& localBid : m_localBids) {
                totalDemand += localBid.getSize();
                demandSuppyTable[localBid.getPrice()].first = totalDemand;
            }
            for (const auto& localAsk : m_localAsks) {
                totalSupply += localAsk.getSize();
                demandSuppyTable[localAsk.getPrice()].second = totalSupply;
            }

            // for debugging:
            // cout << endl;
            // cout << "Price\tDemand\tSupply" << endl;
            // for (const auto& [price, ds] : demandSuppyTable) {
            //     cout << price << '\t' << ds.first << '\t' << ds.second << endl;
            // }
            // cout << endl;

            // now we need to "fill the gaps" of the total demand/supply at individual price levels:
            // some prices may be available in the bid order book, but not in the ask order book
            // (and vice versa)
            // --
            // std::map is ordered, so in the case of the demand (bids),
            // we need to move backwards in the demand/supply table:
            prevValue = 0;
            for (auto rit = demandSuppyTable.rbegin(); rit != demandSuppyTable.rend(); ++rit) {
                if (rit->second.first == 0) {
                    rit->second.first = prevValue;
                } else {
                    prevValue = rit->second.first;
                }
            }
            prevValue = 0;
            for (auto& kv : demandSuppyTable) {
                if (kv.second.second == 0) {
                    kv.second.second = prevValue;
                } else {
                    prevValue = kv.second.second;
                }
            }

            // for debugging:
            // cout << endl;
            // cout << "Price\tDemand\tSupply" << endl;
            // for (const auto& [price, ds] : demandSuppyTable) {
            //     cout << price << '\t' << ds.first << '\t' << ds.second << endl;
            // }
            // cout << endl;

            // apply execution price selection rules
            for (const auto& [price, ds] : demandSuppyTable) {
                minValue = std::min(ds.first, ds.second);
                if (minValue > totalExecutionSize) {
                    // 1st rule: maximum aggregated size of matched orders
                    minExcess = std::abs(ds.first - ds.second);
                    totalExecutionSize = minValue;
                    executionPrice = price;
                } else if (minValue == totalExecutionSize) {
                    absExcess = std::abs(ds.first - ds.second);
                    if (absExcess < minExcess) {
                        // 2nd rule: minimum number of unmatched orders
                        minExcess = absExcess;
                        executionPrice = price;
                    } else if (absExcess == minExcess) {
                        // 3rd rule: closest to last execution price
                        if (std::abs(price - m_lastExecutionPrice) < std::abs(executionPrice - m_lastExecutionPrice)) {
                            executionPrice = price;
                        }
                    }
                } else { // minValue < totalExecutionSize
                    break;
                }
            }

            // for debugging:
            cout << endl;
            cout << "Total Execution Size: " << totalExecutionSize << endl;
            cout << "Execution Price: " << executionPrice << endl;
            cout << endl;

            // save execution price as the last execution price (for 3rd rule)
            m_lastExecutionPrice = executionPrice;

            // in frequent batch auctions mode, the Brokerage Center
            // will just broadcast prices market with decision '5',
            // because traders should not actually know how many
            // trades went on (as opposed to continues markets)
            auto now = TimeSetting::getInstance().simulationTimestamp();
            addExecutionReport({ m_symbol,
                executionPrice,
                totalExecutionSize,
                "T1",
                "T2",
                Order::Type::LIMIT_BUY,
                Order::Type::LIMIT_SELL,
                "O1",
                "O2",
                '5', // decision '5' here means this is a price update
                "SHIFT", // destination is the local market
                now,
                now });

            // -----------------------------------------------------------------
            // 2nd step: matching processs
            // -----------------------------------------------------------------

            thisBidLevel = m_localBids.begin();
            thisAskLevel = m_localAsks.begin();

            auto thisBidOrder = thisBidLevel->begin();
            auto thisAskOrder = thisAskLevel->begin();

            std::deque<int> bidSizes;
            std::deque<int> askSizes;

            while (totalExecutionSize > 0) {
                if (bidSizes.empty()) {
                    bidSizes = s_determineExecutionSizes(totalExecutionSize, *thisBidLevel);
                }
                if (askSizes.empty()) {
                    askSizes = s_determineExecutionSizes(totalExecutionSize, *thisAskLevel);
                }

                while (!bidSizes.empty() && !askSizes.empty()) {
                    int executionSize = std::min(bidSizes[0], askSizes[0]);
                    thisBidLevel->setSize(thisBidLevel->getSize() - executionSize);
                    thisAskLevel->setSize(thisAskLevel->getSize() - executionSize);

                    thisBidOrder->setSize(thisBidOrder->getSize() - executionSize);
                    thisAskOrder->setSize(thisAskOrder->getSize() - executionSize);

                    bidSizes[0] -= executionSize;
                    askSizes[0] -= executionSize;
                    totalExecutionSize -= executionSize;

                    addExecutionReport({ m_symbol,
                        executionPrice,
                        executionSize,
                        thisBidOrder->getTraderID(),
                        thisAskOrder->getTraderID(),
                        thisBidOrder->getType(),
                        thisAskOrder->getType(),
                        thisBidOrder->getOrderID(),
                        thisAskOrder->getOrderID(),
                        '2', // decision '2' means this is a trade record
                        "SHIFT", // destination is the local market
                        thisBidOrder->getTime(),
                        thisAskOrder->getTime() });

                    if (bidSizes[0] == 0) {
                        bidSizes.pop_front();
                        if (thisBidOrder->getSize() == 0) {
                            // remove executed order from local bid order book
                            thisBidOrder = thisBidLevel->erase(thisBidOrder);
                        } else {
                            // or simply move to the next order
                            ++thisBidOrder;
                        }
                    }

                    if (askSizes[0] == 0) {
                        askSizes.pop_front();
                        if (thisAskOrder->getSize() == 0) {
                            // remove executed order from local ask order book
                            thisAskOrder = thisAskLevel->erase(thisAskOrder);
                        } else {
                            // or simply move to the next order
                            ++thisAskOrder;
                        }
                    }
                }

                // remove empty price level from local bid order book
                if (thisBidLevel->empty()) {
                    thisBidLevel = m_localBids.erase(thisBidLevel);
                    thisBidOrder = thisBidLevel->begin();
                }

                // remove empty price level from local ask order book
                if (thisAskLevel->empty()) {
                    thisAskLevel = m_localAsks.erase(thisAskLevel);
                    thisAskOrder = thisAskLevel->begin();
                }
            }
        }
    }
}

void FBAStockMarket::doIncrementAuctionCounters()
{
    for (auto& localBid : m_localBids) { // for each price level
        for (auto& order : localBid) { // for each order in the given price level
            order.incrementAuctionCounter();
        }
    }

    for (auto& localAsk : m_localAsks) { // for each price level
        for (auto& order : localAsk) { // for each order in the given price level
            order.incrementAuctionCounter();
        }
    }
}

void FBAStockMarket::doLocalCancelBid(Order& orderRef)
{
    m_thisPriceLevel = m_localBids.begin();

    if (orderRef.getPrice() > 0.0) { // not a market order
        // find right price level for new order
        while ((m_thisPriceLevel != m_localBids.end()) && (orderRef.getPrice() < m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }
    }

    if (m_thisPriceLevel == m_localBids.end()) { // did not find any orders
        cout << "No such order to be canceled." << endl;

    } else if ((orderRef.getPrice() > 0.0) && (orderRef.getPrice() != m_thisPriceLevel->getPrice())) { // did not find the right price for non-market orders
        cout << "No such order to be canceled." << endl;

    } else {
        m_thisLocalOrder = m_thisPriceLevel->begin();

        // find right order id to cancel
        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getOrderID() != m_thisLocalOrder->getOrderID())) {
            ++m_thisLocalOrder;
        }

        if (m_thisLocalOrder != m_thisPriceLevel->end()) {
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, 0.0, '4'); // cancellation orders have executed price = 0.0

            // FBA: changes to the order book during the order submission stage should not be broadcasted
            // if (m_thisLocalOrder->getType() != Order::Type::MARKET_BUY) { // see FBAStockMarket::insertLocalBid for more info
            //     // broadcast local order book update
            //     addOrderBookUpdate({ OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            //         TimeSetting::getInstance().simulationTimestamp() });
            // }

            // remove completely canceled order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_localBids.erase(m_thisPriceLevel);
            } else {
                // FBA: sort by auction counter and size:
                // this is required when pro-rating an execution in a given price level
                // (needed here in case of partial cancellation)
                s_sortPriceLevel(*m_thisPriceLevel);
            }

        } else { // did not find any order which matched the right order id
            cout << "No such order to be canceled." << endl;
        }
    }
}

void FBAStockMarket::doLocalCancelAsk(Order& orderRef)
{
    m_thisPriceLevel = m_localAsks.begin();

    if (orderRef.getPrice() > 0.0) { // not a market order
        // find right price level for new order
        while ((m_thisPriceLevel != m_localAsks.end()) && (orderRef.getPrice() > m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }
    }

    if (m_thisPriceLevel == m_localAsks.end()) { // did not find any orders
        cout << "No such order to be canceled." << endl;

    } else if ((orderRef.getPrice() > 0.0) && (orderRef.getPrice() != m_thisPriceLevel->getPrice())) { // did not find the right price for non-market orders
        cout << "No such order to be canceled." << endl;

    } else {
        m_thisLocalOrder = m_thisPriceLevel->begin();

        // find right order id to cancel
        while ((m_thisLocalOrder != m_thisPriceLevel->end()) && (orderRef.getOrderID() != m_thisLocalOrder->getOrderID())) {
            ++m_thisLocalOrder;
        }

        if (m_thisLocalOrder != m_thisPriceLevel->end()) {
            int size = m_thisLocalOrder->getSize() - orderRef.getSize();
            executeLocalOrder(orderRef, size, 0.0, '4'); // cancellation orders have executed price = 0.0

            // FBA: changes to the order book during the order submission stage should not be broadcasted
            // if (m_thisLocalOrder->getType() != Order::Type::MARKET_SELL) { // see FBAStockMarket::insertLocalAsk for more info
            //     // broadcast local order book update
            //     addOrderBookUpdate({ OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
            //         TimeSetting::getInstance().simulationTimestamp() });
            // }

            // remove completely canceled order from local order book
            if (size <= 0) {
                m_thisLocalOrder = m_thisPriceLevel->erase(m_thisLocalOrder);
            }

            // remove empty price level from local order book
            if (m_thisPriceLevel->empty()) {
                m_localAsks.erase(m_thisPriceLevel);
            } else {
                // FBA: sort by auction counter and size:
                // this is required when pro-rating an execution in a given price level
                // (needed here in case of partial cancellation)
                s_sortPriceLevel(*m_thisPriceLevel);
            }

        } else { // did not find any order which matched the right order id
            cout << "No such order to be canceled." << endl;
        }
    }
}

void FBAStockMarket::insertLocalBid(Order newBid)
{
    // market orders have to receive "special threatment":
    // - their insertion/exclusion in/from the order book should not be broadcasted (they are hidden)
    // - they must have a price such that they are always guaranteed first place in the order book
    if (newBid.getType() == Order::Type::MARKET_BUY) {
        newBid.setPrice(std::numeric_limits<double>::max());
    }

    if (!m_localBids.empty()) {
        m_thisPriceLevel = m_localBids.begin();

        // find right price level for new order
        while ((m_thisPriceLevel != m_localBids.end()) && (newBid.getPrice() < m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }

        if (newBid.getPrice() == m_thisPriceLevel->getPrice()) { // add to existing price level
            m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + newBid.getSize());
            m_thisPriceLevel->push_back(std::move(newBid));

        } else if (newBid.getPrice() > m_thisPriceLevel->getPrice()) { // new order is new best bid or in between price levels
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newBid.getPrice());
            newPriceLevel.setSize(newBid.getSize());
            newPriceLevel.push_back(std::move(newBid));

            m_thisPriceLevel = m_localBids.insert(m_thisPriceLevel, std::move(newPriceLevel));

        } else { // m_thisPriceLevel == m_localBids.end()
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newBid.getPrice());
            newPriceLevel.setSize(newBid.getSize());
            newPriceLevel.push_back(std::move(newBid));

            m_localBids.push_back(std::move(newPriceLevel));
            m_thisPriceLevel = m_localBids.end();
            --m_thisPriceLevel;
        }

    } else { // m_localBids.empty()
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(newBid.getPrice());
        newPriceLevel.setSize(newBid.getSize());
        newPriceLevel.push_back(std::move(newBid));

        m_localBids.push_back(std::move(newPriceLevel));
        m_thisPriceLevel = m_localBids.begin();
    }

    // FBA: sort by auction counter and size:
    // this is required when pro-rating an execution in a given price level
    s_sortPriceLevel(*m_thisPriceLevel);

    // FBA: changes to the order book during the order submission stage should not be broadcasted
    // if (newBid.getType() != Order::Type::MARKET_BUY) {
    //     // broadcast local order book update
    //     addOrderBookUpdate({ OrderBookEntry::Type::LOC_BID, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
    //         TimeSetting::getInstance().simulationTimestamp() });
    // }
}

void FBAStockMarket::insertLocalAsk(Order newAsk)
{
    // market orders have to receive "special threatment":
    // - their insertion/exclusion in/from the order book should not be broadcasted (they are hidden)
    // - they must have a price such that they are always guaranteed first place in the order book
    if (newAsk.getType() == Order::Type::MARKET_SELL) {
        newAsk.setPrice(std::numeric_limits<double>::min());
    }

    if (!m_localAsks.empty()) {
        m_thisPriceLevel = m_localAsks.begin();

        // find right price level for new order
        while ((m_thisPriceLevel != m_localAsks.end()) && (newAsk.getPrice() > m_thisPriceLevel->getPrice())) {
            ++m_thisPriceLevel;
        }

        if (newAsk.getPrice() == m_thisPriceLevel->getPrice()) { // add to existing price level
            m_thisPriceLevel->setSize(m_thisPriceLevel->getSize() + newAsk.getSize());
            m_thisPriceLevel->push_back(std::move(newAsk));

        } else if (newAsk.getPrice() < m_thisPriceLevel->getPrice()) { // new order is new best ask or in between price levels
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newAsk.getPrice());
            newPriceLevel.setSize(newAsk.getSize());
            newPriceLevel.push_back(std::move(newAsk));

            m_thisPriceLevel = m_localAsks.insert(m_thisPriceLevel, std::move(newPriceLevel));

        } else { // m_thisPriceLevel == m_localAsks.end()
            PriceLevel newPriceLevel;
            newPriceLevel.setPrice(newAsk.getPrice());
            newPriceLevel.setSize(newAsk.getSize());
            newPriceLevel.push_back(std::move(newAsk));

            m_localAsks.push_back(std::move(newPriceLevel));
            m_thisPriceLevel = m_localAsks.end();
            --m_thisPriceLevel;
        }

    } else { // m_localAsks.empty()
        PriceLevel newPriceLevel;
        newPriceLevel.setPrice(newAsk.getPrice());
        newPriceLevel.setSize(newAsk.getSize());
        newPriceLevel.push_back(std::move(newAsk));

        m_localAsks.push_back(std::move(newPriceLevel));
        m_thisPriceLevel = m_localAsks.begin();
    }

    // FBA: sort by auction counter and size:
    // this is required when pro-rating an execution in a given price level
    s_sortPriceLevel(*m_thisPriceLevel);

    // FBA: changes to the order book during the order submission stage should not be broadcasted
    // if (newAsk.getType() != Order::Type::MARKET_SELL) {
    //     // broadcast local order book update
    //     addOrderBookUpdate({ OrderBookEntry::Type::LOC_ASK, m_symbol, m_thisPriceLevel->getPrice(), m_thisPriceLevel->getSize(),
    //         TimeSetting::getInstance().simulationTimestamp() });
    // }
}

} // markets
