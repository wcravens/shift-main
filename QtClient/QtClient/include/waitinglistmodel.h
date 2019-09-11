#pragma once

#include <QAbstractTableModel>
#include <QVariant>
#include <QVector>

// LibCoreClient
#include <Order.h>

struct WaitingListModelItem {
    QString m_id;
    QString m_symbol;
    QString m_price;
    int m_size;
    char m_type;

    WaitingListModelItem()
    {
    }

    WaitingListModelItem(QString id, QString symbol, QString price, int size, char type)
        : m_id(id)
        , m_symbol(symbol)
        , m_price(price)
        , m_size(size)
        , m_type(type)
    {
    }
};

/** 
 * @brief A class of model to be used in QTableView in the ui.
 *        Extends from QAbstractTableModel, use OrderWaitingListModelItem as content.
 */
class WaitingListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    WaitingListModel();

    // pure virtual
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    // virtual
    bool removeRows(const int& begin, const int& rows, const QModelIndex& index);

    bool getCancelOrderByID(QString id, shift::Order& q);
    std::vector<shift::Order> getAllOrders();

signals:
    void setCancelOrder(QString id, QString size);

protected:
    QVector<WaitingListModelItem> m_waiting_list_item;

public slots:
    void resetInternalData();
    void receiveWaitingList(QVector<shift::Order> waiting_list);
    void onClicked(const QModelIndex& index);
};
