#include "Database.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>
#include <cmath>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const QString &message) {
    throw std::runtime_error(message.toStdString());
}
void exec(QSqlQuery &query) {
    if (!query.exec()) fail(query.lastError().text());
}
void sql(QSqlDatabase db, const QString &statement) {
    QSqlQuery query(db);
    if (!query.exec(statement)) fail(query.lastError().text());
}
void validate(const Event &event) {
    if (event.name.trimmed().isEmpty() || event.name.size() > 240)
        fail("Enter an event name between 1 and 240 characters.");
    if (!event.dueDate.isValid() || event.dueDate.year() < 1900 || event.dueDate.year() > 9999)
        fail("Choose a valid due date between 1900 and 9999.");
    if (!statuses.contains(event.status) || !priorities.contains(event.priority))
        fail("Invalid status or priority.");
    if (!std::isfinite(event.hours) || event.hours < 0 || event.hours > 10000)
        fail("Time estimate must be between 0 and 10,000 hours.");
}
void bindEvent(QSqlQuery &query, const Event &event, const QDate &date) {
    query.bindValue(":name", event.name.trimmed());
    query.bindValue(":date", date.toString(Qt::ISODate));
    query.bindValue(":class", event.className);
    query.bindValue(":status", event.status);
    query.bindValue(":priority", event.priority);
    query.bindValue(":hours", event.hours);
    query.bindValue(":recurring", event.recurring ? 1 : 0);
}
}

Database::Database(const QString &fileName) {
    db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    db.setDatabaseName(fileName);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    if (!db.open()) fail("Cannot open planner database: " + db.lastError().text());
    initialize();
}

Database::~Database() {
    const QString connection = db.connectionName();
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

void Database::initialize() {
    sql(db, "PRAGMA foreign_keys=ON");
    QSqlQuery version(db);
    if (!version.exec("PRAGMA user_version") || !version.next())
        fail("Cannot read database version.");
    const int schemaVersion = version.value(0).toInt();
    version.finish();
    if (schemaVersion > 1) fail("This database requires a newer Digi-Planner version.");
    if (schemaVersion == 1) return;
    if (!db.transaction()) fail(db.lastError().text());
    try {
        sql(db, "CREATE TABLE IF NOT EXISTS classes ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE NOT NULL)");
        sql(db, "CREATE TABLE IF NOT EXISTS events ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL,"
                "due_date TEXT NOT NULL, class_name TEXT NOT NULL DEFAULT 'Other',"
                "mark TEXT NOT NULL DEFAULT 'Not Started',"
                "priority TEXT NOT NULL DEFAULT 'Medium',"
                "time_estimate REAL NOT NULL DEFAULT 0 CHECK(time_estimate >= 0),"
                "recurring INTEGER NOT NULL DEFAULT 0,"
                "FOREIGN KEY(class_name) REFERENCES classes(name))");
        const QStringList defaults{"Math", "Science", "History", "English",
                                   "Computer Science", "Art", "Other"};
        for (const auto &name : defaults) {
            QSqlQuery insert(db);
            insert.prepare("INSERT OR IGNORE INTO classes(name) VALUES(?)");
            insert.addBindValue(name);
            exec(insert);
        }
        sql(db, "CREATE INDEX IF NOT EXISTS events_due_date ON events(due_date)");
        sql(db, "PRAGMA user_version=1");
        if (!db.commit()) fail(db.lastError().text());
    } catch (...) {
        db.rollback();
        throw;
    }
}

QStringList Database::classes() const {
    QSqlQuery query(db);
    if (!query.exec("SELECT name FROM classes ORDER BY name COLLATE NOCASE"))
        fail(query.lastError().text());
    QStringList result;
    while (query.next()) result.append(query.value(0).toString());
    return result;
}

QVector<Event> Database::events() const {
    QSqlQuery query(db);
    if (!query.exec("SELECT id,name,due_date,class_name,mark,priority,time_estimate,recurring "
                    "FROM events ORDER BY due_date,id"))
        fail(query.lastError().text());
    QVector<Event> result;
    while (query.next()) {
        result.append({query.value(0).toLongLong(), query.value(1).toString(),
                       QDate::fromString(query.value(2).toString(), Qt::ISODate),
                       query.value(3).toString(), query.value(4).toString(),
                       query.value(5).toString(), query.value(6).toDouble(),
                       query.value(7).toBool()});
    }
    return result;
}

void Database::addEvent(const Event &event) {
    validate(event);
    if (!classes().contains(event.className)) fail("Choose an existing class.");
    const int count = event.recurring ? 8 : 1;
    if (event.dueDate.addDays((count - 1) * 7).year() > 9999)
        fail("The weekly series would go beyond the supported date range.");
    if (!db.transaction()) fail(db.lastError().text());
    try {
        QSqlQuery query(db);
        query.prepare("INSERT INTO events(name,due_date,class_name,mark,priority,time_estimate,recurring)"
                      " VALUES(:name,:date,:class,:status,:priority,:hours,:recurring)");
        for (int i = 0; i < count; ++i) {
            bindEvent(query, event, event.dueDate.addDays(i * 7));
            exec(query);
        }
        if (!db.commit()) fail(db.lastError().text());
    } catch (...) {
        db.rollback();
        throw;
    }
}

void Database::updateEvent(const Event &event) {
    validate(event);
    if (!classes().contains(event.className)) fail("Choose an existing class.");
    QSqlQuery query(db);
    query.prepare("UPDATE events SET name=:name,due_date=:date,class_name=:class,mark=:status,"
                  "priority=:priority,time_estimate=:hours,recurring=:recurring WHERE id=:id");
    bindEvent(query, event, event.dueDate);
    query.bindValue(":id", event.id);
    exec(query);
    if (query.numRowsAffected() != 1) fail("The selected event no longer exists.");
}

void Database::deleteEvent(qint64 id) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM events WHERE id=?");
    query.addBindValue(id);
    exec(query);
    if (query.numRowsAffected() != 1) fail("The selected event no longer exists.");
}

void Database::addClass(const QString &name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed.size() > 80) fail("Enter a class name from 1 to 80 characters.");
    for (const auto &existing : classes())
        if (existing.compare(trimmed, Qt::CaseInsensitive) == 0) fail("That class already exists.");
    QSqlQuery query(db);
    query.prepare("INSERT INTO classes(name) VALUES(?)");
    query.addBindValue(trimmed);
    exec(query);
}

void Database::removeClass(const QString &name) {
    if (name == "Other") fail("Other is the fallback class and cannot be removed.");
    if (!db.transaction()) fail(db.lastError().text());
    try {
        QSqlQuery move(db);
        move.prepare("UPDATE events SET class_name='Other' WHERE class_name=?");
        move.addBindValue(name);
        exec(move);
        QSqlQuery remove(db);
        remove.prepare("DELETE FROM classes WHERE name=?");
        remove.addBindValue(name);
        exec(remove);
        if (remove.numRowsAffected() != 1) fail("The selected class no longer exists.");
        if (!db.commit()) fail(db.lastError().text());
    } catch (...) {
        db.rollback();
        throw;
    }
}
