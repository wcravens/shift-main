#pragma once

#include "include/CandlestickChart/candleplot.h"
#include "include/navigationplot.h"
#include "include/stocklistfiltermodel.h"

#include <QDialog>
#include <QMutex>
#include <QMutexLocker>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStringListModel>
#include <QTimer>

// QWT
#include <qwt_series_data.h>

namespace Ui {
class ChartDialog;
}

/**
 * @brief A struct to represent every candle data sample object.
 */
struct CandleDataSample {
    long long m_timestamp;
    double m_open;
    double m_high;
    double m_low;
    double m_close;

    CandleDataSample()
    {
    }

    CandleDataSample(long long timestamp, double open, double high, double low, double close)
        : m_timestamp(timestamp)
        , m_open(open)
        , m_high(high)
        , m_low(low)
        , m_close(close)
    {
    }
};

class ChartDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChartDialog(QWidget* parent = nullptr);
    ~ChartDialog();

    void start();
    void initializeChart();
    static bool isFromNasdaq(QString real_name);

public slots:
    void setStocklist(const QStringList &stocklist);
    void setTimePeriod(const int &index);
    void setZoomLevel(const int &index);
    void onStockListIndexChanged(const QModelIndex& indexes);
    void receiveData(const QString &symbol, const long long &timestamp, const double &open, const double &high, const double &low, const double &close);
    void refresh();
    void updateXAxis(const long long &time);
    void updateIntervalSelection(const long long &interval);
    void updateZoomSelection(const long long &interval);
    void updateMarker(const long long &, const long long &);

private:
    Ui::ChartDialog* ui;
    CandlePlot *m_candle_plot;
    NavigationPlot *m_navigation_plot;

    QStringListModel* m_stock_list_model; //!< Model to be used in QTableView stocklist.
    StocklistFilterModel* m_filter_model; //!< Filter Model used to implement progressive search feature.
    QVector<long long> m_interval_options; //!< A vector of all options of intervals to select for the chart.
    QVector<long long> m_zoom_options; //!< A vector of all options to zoom level to select.

    std::map<QString, std::vector<CandleDataSample>> m_raw_samples; //!< map of ticker name and their raw candle data
    std::vector<QString> m_current_stocklist;
    QString m_current_symbol; //!< The symbol whose chart is showing.

    int m_current_interval_index; //!< The index of the current selected option interval.
    int m_current_zoom_index; //!< The index of the current selected zoom level.

    QTimer m_timer;
    int m_last_clicked_index = 0; //!< The index of the last clicked stock.
    bool m_is_loading_done; //!< boolean member to show whether all data loading job is done

    QMutex m_mutex;

signals:
    void timeFrameChanged(long long);
    void dataLoaded();
};
