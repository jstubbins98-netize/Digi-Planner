#pragma once

#include "Event.h"
#include <QSqlDatabase>
#include <QVector>

// All failures propagate to the UI; no failed write is represented as saved.
class Database {
public:
    explicit Database(const QString &fileName);
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    QVector<Event> events() const;
    QStringList classes() const;
    void addEvent(const Event &event);
    void updateEvent(const Event &event);
    void deleteEvent(qint64 id);
    void addClass(const QString &name);
    void removeClass(const QString &name);
private:
    QSqlDatabase db;
    void initialize();
};
