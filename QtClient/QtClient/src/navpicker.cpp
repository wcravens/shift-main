#include "include/navpicker.h"

#include <QDateTime>

/**
 * @brief Method to show the tracker text when mouse hoovers over the navigation plot.
 * @param const QPointF& location of mouse
 * @return QwtText the text message to show.
 */
QwtText NavPicker::trackerTextF(const QPointF& pos) const
{
    emit mouseMoved(pos);

    long long timestamp = pos.x();

    QString timeText;
    timeText.append(QDateTime::fromMSecsSinceEpoch(timestamp, Qt::LocalTime).toString("yyyy-MM-dd hh:mm:ss"));

    return QwtText(timeText);
}
