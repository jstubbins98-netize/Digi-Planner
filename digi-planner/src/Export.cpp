#include "Export.h"
#include <QDateTime>
#include <QFont>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QSaveFile>
#include <QTextDocument>
#include <QTextStream>
#include <QStringConverter>
#include <stdexcept>

namespace {
void require(bool condition, const QString &message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}
QString stamp() { return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"); }
}

void Export::text(const QString &path, const QVector<Event> &events) {
    QSaveFile file(path);
    require(file.open(QIODevice::WriteOnly), file.errorString());
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "DIGI-PLANNER\nExported: " << stamp()
        << "\nTotal assignments: " << events.size() << "\n\n";
    for (const auto &event : events) {
        out << event.name << "\nDue: " << event.dueDate.toString(Qt::ISODate)
            << "\nClass: " << event.className << "\nStatus: " << event.status
            << "\nPriority: " << event.priority << "\nTime estimate: "
            << QString::number(event.hours, 'f', 2)
            << " h\nRecurring: " << (event.recurring ? "Yes" : "No")
            << "\n----------------------------------------\n";
    }
    out.flush();
    require(out.status() == QTextStream::Ok, "Could not write the text export.");
    require(file.commit(), file.errorString());
}

void Export::pdf(const QString &path, const QVector<Event> &events) {
    QSaveFile file(path);
    require(file.open(QIODevice::WriteOnly), file.errorString());
    {
        QPdfWriter writer(&file);
        writer.setTitle("Digi-Planner — Assignments");
        writer.setCreator("Digi-Planner");
        writer.setResolution(96);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageOrientation(QPageLayout::Landscape);
        writer.setPageMargins(QMarginsF(12, 12, 12, 12));
        QString html = "<html><body><h1>Digi-Planner</h1><p>Exported: " + stamp()
                       + " &nbsp; | &nbsp; Total assignments: " + QString::number(events.size())
                       + "</p><table width='100%' cellspacing='0' cellpadding='7' border='1'>"
                         "<thead><tr bgcolor='#deebe8'><th>Event Name</th><th>Due Date</th>"
                         "<th>Class</th><th>Status</th><th>Priority</th><th>Time (h)</th>"
                         "<th>Recurring</th></tr></thead><tbody>";
        int row = 0;
        for (const auto &event : events) {
            html += QString("<tr bgcolor='%1'><td>%2</td><td>%3</td><td>%4</td>"
                            "<td>%5</td><td>%6</td><td>%7</td><td>%8</td></tr>")
                        .arg(row++ % 2 ? "#f0f5f4" : "#ffffff",
                             event.name.toHtmlEscaped(), event.dueDate.toString(Qt::ISODate),
                             event.className.toHtmlEscaped(), event.status.toHtmlEscaped(),
                             event.priority.toHtmlEscaped(), QString::number(event.hours, 'f', 2),
                             event.recurring ? "Yes" : "No");
        }
        html += "</tbody></table></body></html>";
        QTextDocument document;
        document.setDefaultFont(QFont("Segoe UI", 10));
        document.setHtml(html);
        document.print(&writer); // QTextDocument paginates and repeats the table header.
    }
    require(file.error() == QFileDevice::NoError, file.errorString());
    require(file.size() > 0, "The PDF could not be generated.");
    require(file.commit(), file.errorString());
}
