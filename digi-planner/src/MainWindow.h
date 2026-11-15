#pragma once
#include "Database.h"
#include <QMainWindow>
#include <functional>

class CalendarView;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(Database &database, const QString &databasePath);
    void showUpcoming();
private:
    Database &db;
    QVector<Event> events;
    QLineEdit *nameInput, *searchInput, *classInput;
    QDateEdit *dateInput;
    QDoubleSpinBox *hoursInput;
    QCheckBox *recurringInput;
    QComboBox *classAdd, *statusAdd, *priorityAdd;
    QComboBox *classFilter, *statusFilter, *classEdit, *statusEdit, *priorityEdit;
    QLabel *workLabel, *selectionLabel;
    QPushButton *deleteButton;
    QTableWidget *table;
    CalendarView *calendar;
    QStackedWidget *views;
    bool refreshing = false;
    void buildUi(const QString &databasePath);
    void reload();
    void renderTable();
    void syncSelection();
    const Event *selectedEvent() const;
    void addEvent();
    void updateSelected();
    void removeSelected();
    void removeClass();
    void exportEvents(bool pdf);
    void attempt(const std::function<void()> &action);
};
