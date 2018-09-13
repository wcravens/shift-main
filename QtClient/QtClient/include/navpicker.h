#pragma once

#include <qwt_plot_picker.h>

class NavPicker : public QwtPlotPicker {
    Q_OBJECT

public:
    NavPicker(int xAxis, int yAxis, QWidget* widget)
        : QwtPlotPicker(xAxis, yAxis, widget)
    {
    }

protected:
    virtual QwtText trackerTextF(const QPointF& pos) const;

signals:
    void mouseMoved(const QPointF& pos) const;
};
