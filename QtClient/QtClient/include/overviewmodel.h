#pragma once

#include "global.h"
#include "qtcoreclient.h"

#include <QAbstractTableModel>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <QPair>


struct OverviewModelItem {
    QString m_symbol;
    QPair<QString, int> m_last_price;
    QPair<QString, int> m_ask_price;
    QPair<QString, int> m_bid_price;
    int m_ask_size;
    int m_bid_size;

    OverviewModelItem()
    {
    }

    OverviewModelItem(QString symbol, QPair<QString, int> last_price, int bid_size, QPair<QString, int> bid_price, QPair<QString, int> ask_price, int ask_size)
        : m_symbol(symbol)
        , m_last_price(last_price)
        , m_ask_price(ask_price)
        , m_bid_price(bid_price)
        , m_ask_size(ask_size)
        , m_bid_size(bid_size)
    {
    }
};

/** 
 * @brief A class of model to be used in QTableView in the ui.
 *        Extends from QAbstractTableModel, use OverviewModelItem as content.
 */
class OverviewModel : public QAbstractTableModel {
    Q_OBJECT

public:
    OverviewModel();

    // pure virtual
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    int findRowIndex(QString symbol);

private:
    QVector<OverviewModelItem> m_prev_overview;
    QVector<OverviewModelItem> m_new_overview;
    QTimer m_timer;

signals:
    void setSendOrder(QString symbol, double price);
    void sentOpenPrice(QString symbol, double openPrice);

public slots:
    void receiveStocklistReady();
    void refresh();
    void onClicked(const QModelIndex& index);

private slots:
    void resetInternalData();

protected:
    QMap<QString, double> m_open_price_list;
};
