#pragma once

#include "include/CandlestickChart/candledataset.h"

// QWT
#include <qwt_plot_picker.h>

/**
 * @brief A class extends from QwtPlotPicker, used to find data point location
 *        from the candlestick chart.
 * QwtPlotPicker is a QwtPicker tailored for selections on a plot canvas.
 * It is set to a x-Axis and y-Axis and translates all pixel coordinates
 * into this coordinate system.
 */
class CandlePlotPicker : public QwtPlotPicker {
    Q_OBJECT
public:
    CandlePlotPicker(int xAxis, int yAxis, QWidget* widget)
        : QwtPlotPicker(xAxis, yAxis, widget)
    {
        m_candle_data_set = NULL;
    }

    void setCandleDataSet(CandleDataSet*);
    virtual QwtText trackerTextF(const QPointF& pos) const;

private:
    CandleDataSet* m_candle_data_set;

signals:
    void mouseMoved(const QPointF& pos) const;
};
