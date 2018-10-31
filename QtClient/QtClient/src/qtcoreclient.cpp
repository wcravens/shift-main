#include "include/qtcoreclient.h"

#include <QDebug>
#include <QString>
#include <QVector>
#include <QRegExp>

using namespace std::chrono_literals;


QStringList QtCoreClient::getStocklist()
{
    return m_stocklist;
}

void QtCoreClient::receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp)
{
    emit updateCandleChart(QString::fromStdString(symbol)
                           , QString::fromStdString(timestamp).toLongLong(nullptr, 10) * 1000
                           , open
                           , high
                           , low
                           , close);
}

/**
 * @brief Method to loop through the entire stocklist and subscribe them all to their orderbook.
 * @param const std::vector<std::string>&: the target stocklist.
 */
void QtCoreClient::adaptStocklist() {
    std::vector<std::string> stocklist = getStockList();
    m_stocklist.clear();

    for (int i = 0; i < stocklist.size(); i++) {
        m_stocklist += QString::fromStdString(stocklist[i]);
    }

    if (is_first_time) {
        this->subAllOrderBook();

        std::thread(
            [this] {
                while (!this->subAllCandleData())
                {
                    qDebug()<< "still doing subAllCandleData";
                    std::this_thread::sleep_for(1s);
                }
            })
            .detach();

        is_first_time = false;
    }

    requestCompanyNames();

    emit stocklistReady();
}

void QtCoreClient::receiveLastPrice(const std::string& symbol) {
    // TODO
}

void QtCoreClient::receivePortfolio(const std::string& symbol)
{
    emit updatePortfolio(symbol);
}

/**
 * @brief Method called when there's an update to the order waiting list.
 */
void QtCoreClient::receiveWaitingList()
{
    QVector<shift::Order> q_waiting_list;
    std::vector<shift::Order> waiting_list = getWaitingList();
    for (shift::Order order : waiting_list)
        q_waiting_list.push_back(order);
    emit updateWaitingList(q_waiting_list);
}
