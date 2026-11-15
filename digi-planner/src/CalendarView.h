#pragma once

#include "Event.h"
#include <QCalendarWidget>
#include <QMap>
#include <QVector>

class CalendarView : public QCalendarWidget {
public:
    explicit CalendarView(QWidget *parent = nullptr);
    void setEvents(const QVector<Event> &events);
protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;
private:
    QMap<QDate, QVector<Event>> byDate;
    void showDay(QDate date);
};
