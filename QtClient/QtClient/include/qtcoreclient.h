#pragma once

#include <QObject>
#include <QStringList>
#include <map>
#include <string>
#include <thread>

// LibCoreClient
#include <CoreClient.h>
#include <Order.h>
#include <FIXInitiator.h>

/**
 * @brief A class extends from CoreClient, act as the bridge between QtCoreClient project
 *        and CoreClient.
 */
class QtCoreClient
        : public QObject
        , public shift::CoreClient
{
    Q_OBJECT

public:
    static QtCoreClient& getInstance();
    QStringList getStocklist();
    void adaptStocklist();
    void subscribeCandleData(const std::vector<std::string>& stocklist);

private:
    QtCoreClient(){}
    QtCoreClient(std::string username) : CoreClient(username){}

    QStringList m_stocklist; //!< All received stocks for this session.
    std::thread m_candledata_loading_thread;
    bool is_first_time = true;

protected:
    void receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) override;
    void receiveLastPrice(const std::string& symbol) override;
    void receivePortfolio(const std::string& symbol) override;
    void receiveWaitingList() override;

    void debugDump(const std::string& message);

signals:
    void stocklistReady();
    void updateCandleChart(QString symbol, long long timestamp, double open, double high, double low, double close);
    //void updateOrderBook(QVector<Order> orderbook);
    void updatePortfolio(std::string symbol);
    void updateWaitingList(QVector<shift::Order> waitingList);
    void acceptLogin();
    void rejectLogin();
};
