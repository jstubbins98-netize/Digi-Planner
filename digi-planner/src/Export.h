#pragma once
#include "Event.h"
#include <QVector>

namespace Export {
void text(const QString &path, const QVector<Event> &events);
void pdf(const QString &path, const QVector<Event> &events);
}
