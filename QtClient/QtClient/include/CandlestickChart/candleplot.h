#pragma once

#include "global.h"
#include "include/CandlestickChart/candledataset.h"
#include "include/CandlestickChart/candleplotpicker.h"

// QWT
#include <qwt_date_scale_draw.h>
#include <qwt_date_scale_engine.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_tradingcurve.h>
#include <qwt_series_data.h>

#include <QMap>
#include <QStringListModel>

class DateScaleDraw;
class CandlePlot;


/**
 * @brief A brief class to customize the QwtDateScaleDraw to show dates/time on the x-axis
 *        of our candlestick plot in the style we want.
 */
class DateScaleDraw : public QwtDateScaleDraw {
public:
    DateScaleDraw(Qt::TimeSpec timeSpec)
        : QwtDateScaleDraw(timeSpec)
    {
        setDateFormat(QwtDate::Millisecond, "mm:ss:zzz");
        setDateFormat(QwtDate::Second, "hh:mm:ss");
        setDateFormat(QwtDate::Minute, "hh:mm:ss");
        setDateFormat(QwtDate::Hour, "hh:mm");
        setDateFormat(QwtDate::Day, "ddd dd MMM");
        setDateFormat(QwtDate::Week, "Www");
        setDateFormat(QwtDate::Month, "MMM");
    }
};

/**
 * @brief A class extends from QwtPlot to draw candlestick chart.
 */
class CandlePlot : public QwtPlot {
    Q_OBJECT

public:
    CandlePlot(QWidget* = nullptr);
    ~CandlePlot();
    void setInterval(long long interval);
    void clearData(QString symbol);
    bool isDataReady(const QString &symbol);
    void updateXAxis(long long time);

public slots:
    void receiveData(QString symbol, long long timestamp, double open, double high, double low, double close);
    void refresh(QString symbol);
    void setZoomBlock(long long level);
    void moved(QPointF position);

signals:
    void intervalChanged(long long);
    void updateMarker(long long, long long);
    void zoomChanged(long long);

private:
    QString m_current_symbol; //!< current showing symbol
    QMap<QString, CandleDataSet*> m_candle_data; //!< key->symbol in QString, value->CandleDataSet*
    QMap<QString, long long> m_last_interval; //!< Time of the last complete time intervals.

    QwtPlotTradingCurve* m_trading_curve; //!< candlestick plot itself
    QwtPlotMarker* m_plot_marker; //!< the vertical line moves around while mouse hoovering
    CandlePlotPicker* m_plot_picker;

    long long m_frequency; //!< current frequency selection
    long long m_zoom_level; //!< current zoom level level (start - end if all selected)
    long long m_zoom_selection; //!< current zoom level selection (0 if all selected)
    long long m_current_start_time; //!< the start time of current selected time frame

    QwtDateScaleDraw* m_date_scale_draw;
    QwtDateScaleEngine* m_date_scale_engine;
    QwtPlotGrid* m_plot_grid;

    bool m_is_zoomed_till_end; //!< boolean to determine whether the plot should update as new data comes in
};
