#include "CalendarView.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QPainter>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <algorithm>

CalendarView::CalendarView(QWidget *parent) : QCalendarWidget(parent) {
    setGridVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    setMinimumDate(QDate(1900, 1, 1));
    setMaximumDate(QDate(9999, 12, 31));
    connect(this, &QCalendarWidget::clicked, this, &CalendarView::showDay);
}

void CalendarView::setEvents(const QVector<Event> &events) {
    byDate.clear();
    for (const auto &event : events) byDate[event.dueDate].append(event);
    updateCells();
}

void CalendarView::paintCell(QPainter *painter, const QRect &rect, QDate date) const {
    QCalendarWidget::paintCell(painter, rect, date);
    const auto found = byDate.constFind(date);
    if (found == byDate.cend()) return;
    const auto &events = found.value();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    const int count = std::min(5, static_cast<int>(events.size()));
    const int start = rect.center().x() - (count * 9) / 2;
    for (int i = 0; i < count; ++i) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(statusColor(events.at(i).status));
        painter->drawEllipse(QPoint(start + i * 9 + 4, rect.bottom() - 7), 3, 3);
    }
    if (events.size() > 5) {
        painter->setPen(palette().color(QPalette::Text));
        QFont font = painter->font();
        font.setPointSize(8);
        painter->setFont(font);
        painter->drawText(rect.adjusted(3, 2, -3, -2), Qt::AlignTop | Qt::AlignRight,
                          QString("+%1").arg(events.size() - 5));
    }
    painter->restore();
}

void CalendarView::showDay(QDate date) {
    QDialog dialog(this);
    dialog.setWindowTitle("Assignments — " + date.toString(Qt::ISODate));
    dialog.resize(600, 420);
    auto *layout = new QVBoxLayout(&dialog);
    auto *details = new QTextBrowser(&dialog);
    QString html = "<h2>" + date.toString("dddd, MMMM d, yyyy") + "</h2>";
    const auto events = byDate.value(date);
    if (events.isEmpty()) html += "<p>No assignments due on this day.</p>";
    for (const auto &event : events) {
        html += QString("<h3>%1</h3><p>Class: %2<br>Status: %3<br>Priority: %4"
                        "<br>Time estimate: %5 h<br>Recurring: %6</p><hr>")
                    .arg(event.name.toHtmlEscaped(), event.className.toHtmlEscaped(),
                         event.status.toHtmlEscaped(), event.priority.toHtmlEscaped(),
                         QString::number(event.hours, 'f', 2), event.recurring ? "Yes" : "No");
    }
    details->setHtml(html);
    layout->addWidget(details);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}
