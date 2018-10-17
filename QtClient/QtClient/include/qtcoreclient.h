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

class QtCoreClient
        : public QObject
        , public shift::CoreClient
{
    Q_OBJECT

public:
    QtCoreClient(QWidget* parent = nullptr)
    {}
    QtCoreClient(std::string username, QObject* parent = nullptr)
        : QObject(parent)
        , CoreClient(username)
    {}

    QStringList getStocklist();
    void adaptStocklist();

protected:
    void receiveCandlestickData(const std::string& symbol, double open, double high, double low, double close, const std::string& timestamp) override;
    void receiveLastPrice(const std::string& symbol) override;
    void receivePortfolio(const std::string& symbol) override;
    void receiveWaitingList() override;

private:

    QStringList m_stocklist; //!< All received stocks for this session.
    bool is_first_time = true;

signals:
    void stocklistReady();
    void updateCandleChart(QString symbol, long long timestamp, double open, double high, double low, double close);
    void updatePortfolio(std::string symbol);
    void updateWaitingList(QVector<shift::Order> waitingList);
    void acceptLogin();
    void rejectLogin();
};
