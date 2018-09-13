#include "include/navigationplot.h"

#include "include/CandlestickChart/candleplot.h"

#include <qwt_picker_machine.h>

#include <QDateTime>

/**
 * @brief A brief class to customize the QwtDateScaleDraw to show dates/time on the x-axis
 *        of our navigation bar in the format we want.
 */
class NavDateScaleDraw : public QwtDateScaleDraw {
public:
    NavDateScaleDraw(Qt::TimeSpec timeSpec)
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

NavigationPlot::NavigationPlot(QWidget* parent)
    : QwtPlot(parent)
{
    setCanvasBackground(Qt::white);

    setMaximumHeight(200);
    setAxisMaxMajor(QwtPlot::yLeft, 1);

    m_dateScaleDraw = new NavDateScaleDraw(Qt::OffsetFromUTC);

    QDateTime curTime = QDateTime::currentDateTime().toUTC();
    QDateTime dt(curTime.date(), curTime.time(), Qt::LocalTime);
    m_dateScaleDraw->setUtcOffset(dt.secsTo(curTime));

    m_dateScaleEngine = new QwtDateScaleEngine(Qt::UTC);

    // set up x axis: time axis
    setAxisScaleDraw(QwtPlot::xBottom, m_dateScaleDraw);
    setAxisScaleEngine(QwtPlot::xBottom, m_dateScaleEngine);

    setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignCenter | Qt::AlignBottom);
    setAxisTitle(QwtPlot::xBottom, QString("Time"));

    QPen gridPen(QColor(171, 171, 171, 150));
    m_plot_grid = new QwtPlotGrid();
    m_plot_grid->setPen(gridPen);
    m_plot_grid->enableY(false);
    m_plot_grid->attach(this);

    QPen curvePen(QColor(78, 108, 182, 200), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    m_curve = new QwtPlotCurve(QString("stock price"));
    m_curve->setOrientation(Qt::Vertical);
    m_curve->setPen(curvePen);
    m_curve->attach(this);

    m_plot_picker = new NavPicker(QwtPlot::xBottom, QwtPlot::yLeft, this->canvas());
    m_plot_picker->setTrackerMode(QwtPicker::AlwaysOn);
    m_plot_picker->setStateMachine(new QwtPickerClickPointMachine);

    QPen mouseChaserPen(Qt::gray, 2);
    m_mouse_chaser_start = new QwtPlotMarker();
    m_mouse_chaser_start->setLineStyle(QwtPlotMarker::VLine);
    m_mouse_chaser_start->setLinePen(mouseChaserPen);
    m_mouse_chaser_start->attach(this);

    m_mouse_chaser_end = new QwtPlotMarker();
    m_mouse_chaser_end->setLineStyle(QwtPlotMarker::VLine);
    m_mouse_chaser_end->setLinePen(mouseChaserPen);
    m_mouse_chaser_end->attach(this);

    QPen markerPen(QColor(36, 84, 160, 150), 5);
    m_start_marker = new QwtPlotMarker();
    m_start_marker->setLineStyle(QwtPlotMarker::VLine);
    m_start_marker->setLinePen(markerPen);
    m_start_marker->attach(this);

    m_end_marker = new QwtPlotMarker();
    m_end_marker->setLineStyle(QwtPlotMarker::VLine);
    m_end_marker->setLinePen(markerPen);
    m_end_marker->attach(this);

    connect(m_plot_picker, &NavPicker::mouseMoved, this, &NavigationPlot::moved);
    connect(m_plot_picker, SIGNAL(selected(QPointF)), this, SLOT(selected(QPointF)));
}

NavigationPlot::~NavigationPlot()
{
}

/**
 * @brief Called for every time out to refresh the content in the navigation plot.
 * @param QString symbol to be plotted.
 */
void NavigationPlot::refresh(QString symbol)
{
    m_current_symbol = symbol;
    m_curve->setSamples(m_time[symbol], m_navdata[symbol]);
    replot();
}

/**
 * @brief Function called when new data received, insert new data into the corresponding vectors.
 * @param QString symbol for the new data
 * @param long long timestamp for the new data
 * @param double average value of open/high/close/low used to plot the curve.
 */
void NavigationPlot::receiveData(QString symbol, long long timestamp, double data)
{
    m_time[symbol].push_back((double)timestamp);
    m_navdata[symbol].push_back(data);
}

/** 
 * @brief Function called when the active stock changed. Reset the data for the curve as the newly changed
 *        stock name.
 * @param QString the symbol to be set.
 */
void NavigationPlot::stockChanged(QString symbol)
{
    m_curve->setSamples(m_time[symbol], m_navdata[symbol]);
}

/**
 * @brief Function triggered when user's mouse has hoovered over the navigation plot. Adjust the location
 *        of the mouse chaser.
 * @param QPointF new location of the mouse
 */
void NavigationPlot::moved(QPointF pos)
{
    double x = pos.x();
    double y = pos.y();

    m_mouse_chaser_start->setValue(x - m_zoom_level / 2, y);
    m_mouse_chaser_end->setValue(x + m_zoom_level / 2, y);

    m_mouse_chaser_start->show();
    m_mouse_chaser_end->show();

    replot();
}

/**
 * @brief Function called when mouse was clicked on the graph, emit a signal to update the candleplot.
 * @param QPointF the clicked location.
 */
void NavigationPlot::selected(QPointF pos)
{
    emit timeFrameSelected(pos.x());
}

/**
 * @brief Update the selected time frame marker according to the received data from candleplot.
 * @param long long start time of the time frame
 * @param long long end time of the time frame
 */
void NavigationPlot::updateMarker(long long start, long long end)
{
    m_start_marker->setValue(start, 0);
    m_end_marker->setValue(end, 0);

    m_start_marker->show();
    m_end_marker->show();

    replot();
}

/** 
 * @brief Function to set zoom level in the navigation plot according to user selection.
 * @param long long level to be set
 */
void NavigationPlot::setZoomBlock(long long level)
{
    if (!level) {
        level = m_time[m_current_symbol].at(m_time[m_current_symbol].size() - 1) - m_time[m_current_symbol].at(0);
    }
    m_zoom_level = level;
}
