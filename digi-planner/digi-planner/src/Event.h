#pragma once

#include <QColor>
#include <QDate>
#include <QStringList>

struct Event {
    qint64 id = 0;
    QString name;
    QDate dueDate;
    QString className = "Other";
    QString status = "Not Started";
    QString priority = "Medium";
    double hours = 0;
    bool recurring = false;
};

inline const QStringList statuses{"Not Started", "In Progress", "Completed", "On Hold"};
inline const QStringList priorities{"High", "Medium", "Low"};

inline QColor statusColor(const QString &status) {
    if (status == "Completed") return QColor("#15803d");
    if (status == "In Progress") return QColor("#c65d08");
    if (status == "On Hold") return QColor("#857000");
    return QColor("#c62828");
}

inline QColor priorityColor(const QString &priority) {
    if (priority == "High") return QColor("#c62828");
    if (priority == "Medium") return QColor("#c65d08");
    return QColor("#15803d");
}
