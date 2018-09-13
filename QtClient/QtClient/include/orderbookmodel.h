#pragma once

#include <QAbstractTableModel>
#include <QVariant>
#include <QVector>

// LibCoreClient
#include <OrderBookEntry.h>


struct OrderBookItem {
    QString m_price;
    int m_size;
    QString m_destination;

    OrderBookItem()
    {
    }

    OrderBookItem(QString price, int size, QString destination)
        : m_price(price)
        , m_size(size)
        , m_destination(destination)
    {
    }
};

class OrderBookModel : public QAbstractTableModel {
    Q_OBJECT

public:
    OrderBookModel();

    // pure virtual
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    // virtual
    bool removeRows(int position, int rows, const QModelIndex& index);

    // user defined
    void updateData(std::vector<shift::OrderBookEntry> entries);

protected:
    QVector<OrderBookItem> m_order_books;

public slots:
    void resetInternalData();
};
