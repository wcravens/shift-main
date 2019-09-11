#include "include/CandlestickChart/candleplot.h"

#include "include/CandlestickChart/candledataset.h"
#include "include/chartdialog.h"
#include "include/qtcoreclient.h"

#include <QDateTime>
#include <QRegExp>
#include <QTimeZone>

CandlePlot::CandlePlot(QWidget* parent)
    : QwtPlot(parent)
{
    setTitle("Candlestick Chart");

    // since data is in GMT, current time is UTC, set offset as 14400
    m_date_scale_draw = new DateScaleDraw(Qt::LocalTime);

    QDateTime utcTime = QDateTime::currentDateTime().toUTC();
    QDateTime localTime(utcTime.date(), utcTime.time(), Qt::LocalTime);
    m_date_scale_draw->setUtcOffset(localTime.secsTo(utcTime));

    m_date_scale_engine = new QwtDateScaleEngine(Qt::UTC);

    // set up x axis: time axis
    setAxisScaleDraw(QwtPlot::xBottom, m_date_scale_draw);
    setAxisScaleEngine(QwtPlot::xBottom, m_date_scale_engine);

    setCanvasBackground(Qt::white);

    // Draw the plot grid.
    QPen gridPen(QColor(171, 171, 171, 150));
    m_plot_grid = new QwtPlotGrid();
    m_plot_grid->setPen(gridPen);
    m_plot_grid->enableX(false);
    m_plot_grid->attach(this);

    // Draw the candlestick plot.
    m_trading_curve = new QwtPlotTradingCurve(QString("stock price"));
    m_trading_curve->setOrientation(Qt::Vertical);
    m_trading_curve->setSymbolExtent(1000);
    m_trading_curve->setMinSymbolWidth(3);
    m_trading_curve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);
    m_trading_curve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, Qt::red);
    m_trading_curve->setSymbolBrush(QwtPlotTradingCurve::Increasing, Qt::green);
    m_trading_curve->attach(this);

    // Initialize the plot picker. Used to show tracker text.
    m_plot_picker = new CandlePlotPicker(QwtPlot::xBottom, QwtPlot::yLeft, this->canvas());
    m_plot_picker->setTrackerMode(QwtPicker::AlwaysOn);
    connect(m_plot_picker, &CandlePlotPicker::mouseMoved, this, &CandlePlot::moved);

    // Draw the vertical plot marker which follows mouse movements.
    QPen markerPen(Qt::gray, 2);
    m_plot_marker = new QwtPlotMarker();
    m_plot_marker->setLineStyle(QwtPlotMarker::VLine);
    m_plot_marker->setLinePen(markerPen);
    m_plot_marker->attach(this);

    m_is_zoomed_till_end = false;

    m_zoom_level = 0;
    m_zoom_selection = 0;
}

CandlePlot::~CandlePlot()
{
}

/**
 * @brief Method to refresh the plot as the timer keep moving.
 * @param QString current symbol shown in the plot. Use as param in the case of symbol changed.
 */
void CandlePlot::refresh(QString symbol)
{
    if (symbol != m_current_symbol)
        m_current_symbol = symbol;

    QString companyName = QString::fromStdString(Global::qt_core_client.getCompanyName(m_current_symbol.toStdString()));
    setTitle(m_current_symbol + (companyName == "" ? "" : " (" + companyName + ")"));

    qDebug() << m_current_symbol;
    long long firstTimestamp = m_candle_data[m_current_symbol]->d_samples[0].time;
    long long lastTimestamp = m_candle_data[m_current_symbol]->d_samples[m_candle_data[m_current_symbol]->d_samples.size() - 1].time;

    // if change to a new zoom level, keep refreshing the plot as new data comes in.
    if (m_is_zoomed_till_end) {
        if (m_current_start_time == firstTimestamp)
            setAxisAutoScale(QwtPlot::xBottom);
        else {
            setAxisScale(QwtPlot::xBottom, m_current_start_time, lastTimestamp);
        }

        emit updateMarker(m_current_start_time, m_candle_data[m_current_symbol]->d_samples[m_candle_data[m_current_symbol]->d_samples.size() - 1].time);
    }

    /** When "All" is selected in zoom option:
     * set frequency to 1 seconds when there's less than 10 minutes of data;
     * set frequency to 3 seconds when there's less than 30 minutes of data;
     * set frequency to 10 seconds when there's less than 1 hour of data;
     * set frequency to 30 seconds when there's more than 1 hour of data.
     */
    long long passedTime = lastTimestamp - firstTimestamp;
    if (!m_zoom_selection) {
        long long newFrequency;
        if (passedTime < 600000)
            newFrequency = 1000;
        else if (passedTime < 1800000)
            newFrequency = 3000;
        else if (passedTime < 3600000)
            newFrequency = 10000;
        else
            newFrequency = 30000;

        setInterval(newFrequency);
        emit intervalChanged(m_frequency);
    }

    m_trading_curve->setSamples(m_candle_data[m_current_symbol]->d_samples);
    m_plot_picker->setCandleDataSet(m_candle_data[m_current_symbol]);

    replot();
}

/**
 * @brief Method called when new data comes in.
 */
void CandlePlot::receiveData(QString symbol, long long timestamp,
    double open, double high, double low, double close)
{
    if (!m_last_interval[symbol])
        m_last_interval[symbol] = timestamp - timestamp % m_frequency;

    if (!m_candle_data[symbol])
        m_candle_data[symbol] = new CandleDataSet(symbol);

    if (timestamp - m_last_interval[symbol] < m_frequency) {
        QDateTime qtime = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::LocalTime);
        double qwtTime = QwtDate::toDouble(qtime);

        QwtOHLCSample sample(qwtTime, open, high, low, close);
        m_candle_data[symbol]->update(sample);
    } else {
        m_last_interval[symbol] += m_frequency;
        QDateTime qtime = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::LocalTime);
        double qwtTime = QwtDate::toDouble(qtime);

        m_candle_data[symbol]->append(QwtOHLCSample(qwtTime, open, high, low, close));
    }
}

/**
 * @brief Method to set interval/frequency of current plot.
 * @param long long the new interval to be set
 */
void CandlePlot::setInterval(long long interval)
{
    m_frequency = interval;
    m_trading_curve->setSymbolExtent(interval);
}

/**
 * @brief Used to clear the current on-show data and reset the scale for xBottom axis.
 * @param QString symbol of the dataset to be cleared.
 */
void CandlePlot::clearData(QString symbol)
{
    CandleDataSet* candleDataSet = m_candle_data[symbol];
    if (!candleDataSet) {
        qDebug() << "no such symbol: " << symbol;
        return;
    }
    candleDataSet->d_samples.clear();
    m_last_interval[symbol] = 0;
    setAxisAutoScale(QwtPlot::xBottom);
}

bool CandlePlot::isDataReady(const QString& symbol)
{
    if (m_candle_data[symbol])
        return true;
    else
        return false;
}

/**
 * @brief Method to update scale of x axis when the plot is zoomed.
 * @param long long the clicked time
 */
void CandlePlot::updateXAxis(long long time)
{
    // If "All" is currently selected.
    if (!m_zoom_level) {
        m_zoom_level = 3600000;
        setAxisAutoScale(QwtPlot::xBottom);
        emit updateMarker(0, 0);
        emit zoomChanged(m_zoom_level);
        setInterval(1000);
        emit intervalChanged(1000);
        return;
    }

    long long start = time - m_zoom_level / 2;
    long long end = time + m_zoom_level / 2;

    long long firstTime = m_candle_data[m_current_symbol]->d_samples[0].time;
    long long lastTime = m_candle_data[m_current_symbol]->d_samples[m_candle_data[m_current_symbol]->d_samples.size() - 1].time;

    // If selected start time is earlier than the time of the first data, set start as the first data.
    if (start < firstTime) {
        start = firstTime;
        // start + 1 hour
        end = start + m_zoom_level;
        m_is_zoomed_till_end = false;
    } else if (end >= lastTime && lastTime - firstTime - 10000 > m_zoom_level) {
        // end - 1 hour
        start = lastTime - m_zoom_level;
        end = lastTime;

        m_current_start_time = start;

        m_is_zoomed_till_end = true;
    } else {
        m_is_zoomed_till_end = false;
    }

    // emit the signal to update the marker in navigation bar
    emit updateMarker(start, end);

    // set axis scale for x axis and set interval accordingly
    setAxisScale(QwtPlot::xBottom, start, end);
    setInterval(m_zoom_level / 60);
    emit intervalChanged(m_zoom_level / 60);
}

/**
 * @brief Method to set the zoom block/marker for both candle plot and nav bar.
 * @param long long the new zoom level to be set
 */
void CandlePlot::setZoomBlock(long long level)
{
    // level == 0 if "All" is selected.
    if (!level) {
        long long start = m_candle_data[m_current_symbol]->d_samples[0].time;
        long long end = m_candle_data[m_current_symbol]->d_samples[m_candle_data[m_current_symbol]->d_samples.size() - 1].time;
        m_current_start_time = start;
        m_zoom_level = end - start;
        m_zoom_selection = level;

        long long newFrequency;
        if (m_zoom_level < 600000)
            newFrequency = 1000;
        else if (m_zoom_level < 1800000)
            newFrequency = 3000;
        else if (m_zoom_level < 3600000)
            newFrequency = 10000;
        else
            newFrequency = 30000;

        setInterval(newFrequency);
        emit intervalChanged(m_frequency);

        setAxisScale(QwtPlot::xBottom, start, end);
        m_is_zoomed_till_end = true;
    } else {
        long long lastTime = m_candle_data[m_current_symbol]->d_samples[m_candle_data[m_current_symbol]->d_samples.size() - 1].time;
        setAxisScale(QwtPlot::xBottom, lastTime - level, lastTime);

        m_current_start_time = lastTime - level;

        emit updateMarker(lastTime - level, lastTime);

        m_zoom_selection = level;
        m_zoom_level = level;
        setInterval(m_zoom_level / 60);

        emit intervalChanged(m_zoom_level / 60);

        m_is_zoomed_till_end = true;
    }
}

/**
 * @brief Method to update vertical plot marker when mouse moved over the plot
 * @param QPointF new location of mouse
 */
void CandlePlot::moved(QPointF position)
{
    m_plot_marker->setValue(position.x(), position.y());

    m_plot_marker->show();
    replot();
}
