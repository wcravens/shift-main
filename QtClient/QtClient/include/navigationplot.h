#pragma once

#include "include/navpicker.h"

#include <QMouseEvent>
#include <QPointF>
#include <QRubberBand>

// QWT
#include <qwt_date_scale_draw.h>
#include <qwt_date_scale_engine.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

class NavigationPlot : public QwtPlot {
    Q_OBJECT
public:
    NavigationPlot(QWidget* = NULL);
    ~NavigationPlot();

    void clearData(QString symbol);
    void stockChanged(QString symbol);
    void updateMarker(long long, long long);
    void setZoomBlock(long long level);

public slots:
    void receiveData(QString qsymbol, long long timestamp, double data);
    void refresh(QString symbol);
    void moved(QPointF pos);
    void selected(QPointF pos);

signals:
    void timeFrameSelected(long long time);

private:
    QString m_current_symbol;
    QMap<QString, QVector<double>> m_navdata; //!< key->symbol in QString, value->average of open/close/high/low
    QMap<QString, QVector<double>> m_time; //!< key->symbol in QString, value->sample timestamp

    QwtPlotCurve* m_curve;

    QwtDateScaleDraw* m_dateScaleDraw;
    QwtDateScaleEngine* m_dateScaleEngine;

    NavPicker* m_plot_picker;
    QwtPlotMarker* m_mouse_chaser_start;
    QwtPlotMarker* m_mouse_chaser_end;

    QwtPlotMarker* m_start_marker;
    QwtPlotMarker* m_end_marker;

    QwtPlotGrid* m_plot_grid;

    long long m_zoom_level; //!< current selected zoom level
};
