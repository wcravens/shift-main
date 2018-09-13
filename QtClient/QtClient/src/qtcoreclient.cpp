#include "include/qtcoreclient.h"

#include <QDebug>
#include <QString>
#include <QVector>
#include <QRegExp>


/** 
 * @brief Method to get the singleton pointer around the project.
 * @return QtCoreClient*: The pointer to the current QtCoreClient.
 */
QtCoreClient& QtCoreClient::getInstance()
{
    static QtCoreClient instance;
    return instance;
}

/**
 * @brief Method to return current stocklist as a QStringList object.
 * @return QStringList: current stocklist in the form of QStringList.
 */
QStringList QtCoreClient::getStocklist()
{
    return m_stocklist;
}

/**
 * @brief Method called when new candle data received.
 * @param std::string: symbol of the new data.
 * @param double: open/high/low/close of the new data.
 * @param std::string: timestamp of the new data.
 */
void QtCoreClient::receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp)
{
    QString qsymbol = QString::fromStdString(symbol);
    QString time_str = QString::fromStdString(timestamp);
    bool ok;
    long long lltimestamp = time_str.toLongLong(&ok, 10) * 1000;
    emit updateCandleChart(qsymbol, lltimestamp, open, high, low, close);
}

/**
 * @brief Method to loop through the entire stocklist and subscribe them all to their orderbook.
 * @param const std::vector<std::string>&: the target stocklist.
 */
void QtCoreClient::adaptStocklist() {
    std::vector<std::string> stocklist = QtCoreClient::getInstance().getStockList();
    m_stocklist.clear();

    for (int i = 0; i < stocklist.size(); i++) {
//        qDebug() << QString::fromStdString(stocklist[i]);
        m_stocklist += QString::fromStdString(stocklist[i]);

        // subscribe orderbook
        if (is_first_time)
            QtCoreClient::getInstance().subOrderBook(stocklist[i]);
    }

    if (is_first_time) {
        m_candledata_loading_thread = std::thread(&QtCoreClient::subscribeCandleData, this, stocklist);
        is_first_time = false;
    }

    QtCoreClient::getInstance().requestCompanyNames();

    emit stocklistReady();
}

void QtCoreClient::subscribeCandleData(const std::vector<std::string> &stocklist) {
    for (int i = 0; i < stocklist.size(); i++) {
        QtCoreClient::getInstance().subCandleData(stocklist[i]);
    }
}

void QtCoreClient::receiveLastPrice(const std::string& symbol) {
    // TODO
//    std::string s = symbol;
}

/**
 * @brief Method called when there's an update to the portfolio.
 * @param double total year to date PL.
 * @param double Buying Power
 * @param int total shares traded
 * @param std::string updated symbol
 * @param int traded shares for the symbol
 * @param double trading price 
 * @param double PL for the current symbol.
 * @param std::string current username.
 */
void QtCoreClient::receivePortfolio(const std::string& symbol)
{
    emit updatePortfolio(symbol);
}

/**
 * @brief Method called when there's an update to the order waiting list.
 * @param const std::vector<shift::Order>&: new waiting list.
 * @param std::string: current username.
 */
void QtCoreClient::receiveWaitingList()
{
    QVector<shift::Order> q_waiting_list;
    std::vector<shift::Order> waiting_list = QtCoreClient::getInstance().getWaitingList();
    for (shift::Order order : waiting_list) {
        q_waiting_list.push_back(order);
    }
    emit updateWaitingList(q_waiting_list);
}

/**
 * @brief Method to print log debug message.
 * @param const std::string&: the message to be printed.
 */
void QtCoreClient::debugDump(const std::string& message)
{
    qDebug() << QString::fromStdString(message);
}
