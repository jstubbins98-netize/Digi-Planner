#include "MainWindow.h"
#include "CalendarView.h"
#include "Export.h"
#include "Tutorial.h"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QKeySequence>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <exception>

namespace {
class HoursItem : public QTableWidgetItem {
public:
    explicit HoursItem(double hours)
        : QTableWidgetItem(hours == 0 ? "-" : QString::number(hours, 'f', 2)), value(hours) {}
    bool operator<(const QTableWidgetItem &other) const override {
        const auto *hours = dynamic_cast<const HoursItem *>(&other);
        return hours ? value < hours->value : QTableWidgetItem::operator<(other);
    }
private:
    double value;
};

QComboBox *combo(const QStringList &items, QWidget *parent) {
    auto *widget = new QComboBox(parent);
    widget->addItems(items);
    return widget;
}
void restoreCombo(QComboBox *widget, const QStringList &items, const QString &fallback) {
    const QSignalBlocker blocker(widget);
    const QString current = widget->currentText();
    widget->clear();
    widget->addItems(items);
    widget->setCurrentText(items.contains(current) ? current : fallback);
}
void field(QGridLayout *layout, const QString &label, QWidget *widget, int column) {
    auto *caption = new QLabel(label);
    caption->setBuddy(widget);
    layout->addWidget(caption, 0, column);
    layout->addWidget(widget, 1, column);
}
}

MainWindow::MainWindow(Database &database, const QString &databasePath) : db(database) {
    buildUi(databasePath);
    reload();
}

void MainWindow::buildUi(const QString &databasePath) {
    setWindowTitle("Digi-Planner");
    auto *helpMenu = menuBar()->addMenu("&Help");
    auto *tutorialAction = helpMenu->addAction("&Tutorial");
    tutorialAction->setShortcut(QKeySequence(Qt::Key_F1));
    connect(tutorialAction, &QAction::triggered, this, [this] { Tutorial::show(this); });
    resize(1180, 800);
    setMinimumSize(980, 680);
    auto *root = new QWidget(this);
    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(14);
    setCentralWidget(root);

    auto *heading = new QHBoxLayout;
    auto *title = new QLabel("Digi-Planner\nYour assignments. A clearer week.", root);
    title->setObjectName("heading");
    heading->addWidget(title, 1);
    auto *tutorialButton = new QPushButton("Tutorial", root);
    tutorialButton->setToolTip("Learn how to use Digi-Planner (F1)");
    heading->addWidget(tutorialButton);
    connect(tutorialButton, &QPushButton::clicked, this, [this] { Tutorial::show(this); });
    auto *switchView = new QPushButton("Switch to Calendar View", root);
    heading->addWidget(switchView);
    layout->addLayout(heading);

    views = new QStackedWidget(root);
    auto *listPage = new QWidget(views);
    auto *listLayout = new QVBoxLayout(listPage);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(12);
    views->addWidget(listPage);
    calendar = new CalendarView(views);
    views->addWidget(calendar);
    layout->addWidget(views, 1);
    connect(switchView, &QPushButton::clicked, this, [this, switchView] {
        const bool toCalendar = views->currentIndex() == 0;
        views->setCurrentIndex(toCalendar ? 1 : 0);
        switchView->setText(toCalendar ? "Switch to List View" : "Switch to Calendar View");
    });

    auto *addGroup = new QGroupBox("Add New Event", listPage);
    auto *addLayout = new QGridLayout(addGroup);
    nameInput = new QLineEdit(addGroup);
    nameInput->setPlaceholderText("Assignment name");
    nameInput->setMaxLength(240);
    dateInput = new QDateEdit(QDate::currentDate(), addGroup);
    dateInput->setDisplayFormat("yyyy-MM-dd");
    dateInput->setCalendarPopup(true);
    dateInput->setDateRange(QDate(1900, 1, 1), QDate(9999, 12, 31));
    statusAdd = combo(statuses, addGroup);
    priorityAdd = combo(priorities, addGroup);
    priorityAdd->setCurrentText("Medium");
    hoursInput = new QDoubleSpinBox(addGroup);
    hoursInput->setRange(0, 10000);
    hoursInput->setDecimals(2);
    hoursInput->setSingleStep(0.25);
    classAdd = combo({}, addGroup);
    field(addLayout, "&Event Name", nameInput, 0);
    field(addLayout, "&Due Date", dateInput, 1);
    field(addLayout, "&Status", statusAdd, 2);
    field(addLayout, "&Priority", priorityAdd, 3);
    field(addLayout, "&Time Estimate (h)", hoursInput, 4);
    field(addLayout, "&Class", classAdd, 5);
    addLayout->setColumnStretch(0, 2);
    recurringInput = new QCheckBox("Recurring Weekly (7 weeks)", addGroup);
    recurringInput->setToolTip("Creates 8 independent entries: this date plus 7 weekly occurrences.");
    addLayout->addWidget(recurringInput, 2, 0, 1, 4);
    auto *addButton = new QPushButton("Add Event", addGroup);
    addButton->setObjectName("primary");
    addLayout->addWidget(addButton, 2, 4, 1, 2);
    listLayout->addWidget(addGroup);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addEvent);
    connect(nameInput, &QLineEdit::returnPressed, this, &MainWindow::addEvent);

    auto *classGroup = new QGroupBox("Class Management", listPage);
    auto *classLayout = new QHBoxLayout(classGroup);
    classInput = new QLineEdit(classGroup);
    classInput->setPlaceholderText("New Class");
    classInput->setAccessibleName("New class name");
    classInput->setMaxLength(80);
    classLayout->addWidget(classInput, 1);
    auto *addClassButton = new QPushButton("Add", classGroup);
    auto *removeClassButton = new QPushButton("Remove Classes...", classGroup);
    classLayout->addWidget(addClassButton);
    classLayout->addWidget(removeClassButton);
    listLayout->addWidget(classGroup);
    const auto addClass = [this] {
        attempt([this] {
            db.addClass(classInput->text());
            classInput->clear();
            reload();
            statusBar()->showMessage("Class added.", 4000);
        });
    };
    connect(addClassButton, &QPushButton::clicked, this, addClass);
    connect(classInput, &QLineEdit::returnPressed, this, addClass);
    connect(removeClassButton, &QPushButton::clicked, this, &MainWindow::removeClass);

    auto *filters = new QHBoxLayout;
    searchInput = new QLineEdit(listPage);
    searchInput->setPlaceholderText("Search assignments...");
    searchInput->setAccessibleName("Search assignment names");
    classFilter = combo({"All"}, listPage);
    classFilter->setAccessibleName("Filter by class");
    statusFilter = combo(QStringList{"All"} + statuses, listPage);
    statusFilter->setAccessibleName("Filter by status");
    auto *clearFilters = new QPushButton("Clear Filters", listPage);
    filters->addWidget(searchInput, 2);
    filters->addWidget(new QLabel("Class:", listPage));
    filters->addWidget(classFilter, 1);
    filters->addWidget(new QLabel("Status:", listPage));
    filters->addWidget(statusFilter, 1);
    filters->addWidget(clearFilters);
    listLayout->addLayout(filters);
    const auto filter = [this] { if (!refreshing) renderTable(); };
    connect(searchInput, &QLineEdit::textChanged, this, filter);
    connect(classFilter, &QComboBox::currentIndexChanged, this, filter);
    connect(statusFilter, &QComboBox::currentIndexChanged, this, filter);
    connect(clearFilters, &QPushButton::clicked, this, [this] {
        { const QSignalBlocker a(searchInput), b(classFilter), c(statusFilter);
          searchInput->clear(); classFilter->setCurrentIndex(0); statusFilter->setCurrentIndex(0); }
        renderTable();
    });

    auto *legend = new QLabel(
        "<b>Status</b> &nbsp; <span style='color:#c62828'>● Not Started</span> &nbsp; "
        "<span style='color:#c65d08'>● In Progress</span> &nbsp; "
        "<span style='color:#15803d'>● Completed</span> &nbsp; "
        "<span style='color:#857000'>● On Hold</span><br>"
        "<b>Priority</b> &nbsp; <span style='color:#c62828'>★ High</span> &nbsp; "
        "<span style='color:#c65d08'>★ Medium</span> &nbsp; "
        "<span style='color:#15803d'>★ Low</span>", listPage);
    listLayout->addWidget(legend);
    workLabel = new QLabel(listPage);
    listLayout->addWidget(workLabel);
    table = new QTableWidget(0, 7, listPage);
    table->setHorizontalHeaderLabels({"Status", "Name", "Due Date", "Class",
                                     "Priority", "Time (h)", "Recurring"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(36);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setSortingEnabled(true);
    table->sortItems(2, Qt::AscendingOrder);
    listLayout->addWidget(table, 1);
    connect(table, &QTableWidget::itemSelectionChanged, this, &MainWindow::syncSelection);

    auto *actions = new QGroupBox("Selected assignment — changes save immediately", listPage);
    auto *actionLayout = new QVBoxLayout(actions);
    selectionLabel = new QLabel("Select an assignment to edit its class, status or priority.", actions);
    selectionLabel->setTextFormat(Qt::PlainText);
    actionLayout->addWidget(selectionLabel);
    auto *row = new QHBoxLayout;
    classEdit = combo({}, actions);
    statusEdit = combo(statuses, actions);
    priorityEdit = combo(priorities, actions);
    classEdit->setAccessibleName("Selected assignment class");
    statusEdit->setAccessibleName("Selected assignment status");
    priorityEdit->setAccessibleName("Selected assignment priority");
    row->addWidget(new QLabel("Class:", actions));
    row->addWidget(classEdit, 1);
    row->addWidget(new QLabel("Status:", actions));
    row->addWidget(statusEdit, 1);
    row->addWidget(new QLabel("Priority:", actions));
    row->addWidget(priorityEdit, 1);
    deleteButton = new QPushButton("Delete Selected", actions);
    row->addWidget(deleteButton);
    actionLayout->addLayout(row);
    listLayout->addWidget(actions);
    for (auto *widget : {classEdit, statusEdit, priorityEdit})
        connect(widget, &QComboBox::currentIndexChanged, this, &MainWindow::updateSelected);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::removeSelected);

    auto *footer = new QHBoxLayout;
    auto *storageLabel = new QLabel("Saved automatically on this PC", root);
    storageLabel->setToolTip(databasePath);
    footer->addWidget(storageLabel, 1);
    auto *txt = new QPushButton("Export TXT", root);
    auto *pdf = new QPushButton("Export PDF", root);
    footer->addWidget(txt);
    footer->addWidget(pdf);
    layout->addLayout(footer);
    connect(txt, &QPushButton::clicked, this, [this] { exportEvents(false); });
    connect(pdf, &QPushButton::clicked, this, [this] { exportEvents(true); });
}

void MainWindow::attempt(const std::function<void()> &action) {
    try { action(); }
    catch (const std::exception &error) {
        refreshing = false;
        QMessageBox message(QMessageBox::Critical, "Action could not be completed",
                            QString::fromUtf8(error.what()), QMessageBox::Ok, this);
        message.setTextFormat(Qt::PlainText);
        message.exec();
    }
}

void MainWindow::reload() {
    const auto loadedEvents = db.events();
    const auto classes = db.classes();
    events = loadedEvents;
    refreshing = true;
    restoreCombo(classAdd, classes, "Other");
    restoreCombo(classEdit, classes, "Other");
    restoreCombo(classFilter, QStringList{"All"} + classes, "All");
    refreshing = false;
    calendar->setEvents(events);
    renderTable();
}

void MainWindow::renderTable() {
    qint64 selected = 0;
    if (const auto *event = selectedEvent()) selected = event->id;
    const QSignalBlocker blocker(table);
    table->setSortingEnabled(false);
    table->setRowCount(0);
    double pending = 0;
    for (const auto &event : events) {
        if (!event.name.contains(searchInput->text(), Qt::CaseInsensitive)) continue;
        if (classFilter->currentIndex() > 0 && event.className != classFilter->currentText()) continue;
        if (statusFilter->currentIndex() > 0 && event.status != statusFilter->currentText()) continue;
        const int row = table->rowCount();
        table->insertRow(row);
        const QStringList cells{"● " + event.status, event.name, event.dueDate.toString(Qt::ISODate),
                                event.className, "★ " + event.priority, "",
                                event.recurring ? "🔄 Yes" : "No"};
        for (int column = 0; column < cells.size(); ++column) {
            QTableWidgetItem *item = column == 5
                ? static_cast<QTableWidgetItem *>(new HoursItem(event.hours))
                : new QTableWidgetItem(cells.at(column));
            item->setData(Qt::UserRole, event.id);
            if (column == 0) item->setForeground(statusColor(event.status));
            if (column == 4) item->setForeground(priorityColor(event.priority));
            if (column == 5) {
                item->setToolTip(event.hours == 0 ? "No time estimate set" : "Estimated hours");
            }
            table->setItem(row, column, item);
        }
        if (event.status != "Completed") pending += event.hours;
    }
    table->setSortingEnabled(true);
    table->clearSelection();
    for (int row = 0; row < table->rowCount(); ++row)
        if (table->item(row, 0)->data(Qt::UserRole).toLongLong() == selected) table->selectRow(row);
    workLabel->setText(QString("%1 visible assignments  •  Total pending work: %2 h%3")
                          .arg(table->rowCount()).arg(pending, 0, 'f', 2)
                          .arg(table->rowCount() == 0 ? "  —  Add an event or clear your filters." : ""));
    syncSelection();
}

const Event *MainWindow::selectedEvent() const {
    const auto rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return nullptr;
    const auto *item = table->item(rows.first().row(), 0);
    if (!item) return nullptr;
    const qint64 id = item->data(Qt::UserRole).toLongLong();
    for (const auto &event : events) if (event.id == id) return &event;
    return nullptr;
}

void MainWindow::syncSelection() {
    const Event *event = selectedEvent();
    const QSignalBlocker a(classEdit), b(statusEdit), c(priorityEdit);
    for (auto *widget : {classEdit, statusEdit, priorityEdit}) widget->setEnabled(event != nullptr);
    deleteButton->setEnabled(event != nullptr);
    selectionLabel->setText(event ? event->name : "Select an assignment to edit its class, status or priority.");
    if (event) {
        classEdit->setCurrentText(event->className);
        statusEdit->setCurrentText(event->status);
        priorityEdit->setCurrentText(event->priority);
    }
}

void MainWindow::addEvent() {
    attempt([this] {
        const Event event{0, nameInput->text(), dateInput->date(), classAdd->currentText(),
                          statusAdd->currentText(), priorityAdd->currentText(),
                          hoursInput->value(), recurringInput->isChecked()};
        db.addEvent(event);
        nameInput->clear();
        recurringInput->setChecked(false);
        hoursInput->setValue(0);
        reload();
        statusBar()->showMessage(event.recurring ? "8 weekly assignments saved." : "Assignment saved.", 5000);
        nameInput->setFocus();
    });
}

void MainWindow::updateSelected() {
    if (refreshing) return;
    const Event *selected = selectedEvent();
    if (!selected) return;
    Event updated = *selected;
    updated.className = classEdit->currentText();
    updated.status = statusEdit->currentText();
    updated.priority = priorityEdit->currentText();
    attempt([this, updated] {
        db.updateEvent(updated);
        reload();
        statusBar()->showMessage("Changes saved.", 3000);
    });
    syncSelection(); // restore saved values if a write failed
}

void MainWindow::removeSelected() {
    const Event *event = selectedEvent();
    if (!event) return;
    const qint64 id = event->id;
    QMessageBox prompt(QMessageBox::Question, "Delete assignment?",
                       "Permanently delete \"" + event->name + "\"?\n"
                       "Only this occurrence will be deleted. This cannot be undone.",
                       QMessageBox::Yes | QMessageBox::No, this);
    prompt.setTextFormat(Qt::PlainText);
    prompt.setDefaultButton(QMessageBox::No);
    if (prompt.exec() != QMessageBox::Yes) return;
    attempt([this, id] { db.deleteEvent(id); reload(); });
}

void MainWindow::removeClass() {
    attempt([this] {
        QDialog dialog(this);
        dialog.setWindowTitle("Remove Classes");
        dialog.resize(420, 380);
        auto *layout = new QVBoxLayout(&dialog);
        auto *note = new QLabel("Assignments in a removed class move to Other.\nOther cannot be removed.", &dialog);
        note->setWordWrap(true);
        layout->addWidget(note);
        auto *list = new QListWidget(&dialog);
        auto names = db.classes();
        names.removeAll("Other");
        list->addItems(names);
        layout->addWidget(list);
        auto *remove = new QPushButton("Remove Selected", &dialog);
        remove->setEnabled(false);
        layout->addWidget(remove);
        connect(list, &QListWidget::currentRowChanged, &dialog,
                [remove](int row) { remove->setEnabled(row >= 0); });
        connect(remove, &QPushButton::clicked, &dialog, [this, list, &dialog] {
            if (!list->currentItem()) return;
            const QString name = list->currentItem()->text();
            QMessageBox prompt(QMessageBox::Question, "Remove class?",
                               "Remove \"" + name + "\" and move its assignments to Other?",
                               QMessageBox::Yes | QMessageBox::No, &dialog);
            prompt.setTextFormat(Qt::PlainText);
            prompt.setDefaultButton(QMessageBox::No);
            if (prompt.exec() != QMessageBox::Yes) return;
            attempt([this, list, name] {
                db.removeClass(name);
                reload();
                delete list->takeItem(list->currentRow());
            });
        });
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        dialog.exec();
    });
}

void MainWindow::exportEvents(bool pdf) {
    const QString extension = pdf ? ".pdf" : ".txt";
    const QString path = QFileDialog::getSaveFileName(
        this, "Export all assignments", "Digi-Planner-" + QDate::currentDate().toString(Qt::ISODate) + extension,
        pdf ? "PDF document (*.pdf)" : "Text file (*.txt)");
    if (path.isEmpty()) return;
    attempt([this, path, pdf] {
        const auto all = db.events(); // Exports always include all events, regardless of filters or table sort.
        if (pdf) Export::pdf(path, all); else Export::text(path, all);
        statusBar()->showMessage("Export saved: " + path, 8000);
    });
}

void MainWindow::showUpcoming() {
    attempt([this] {
        QStringList lines;
        const QDate today = QDate::currentDate();
        for (const auto &event : db.events()) {
            const qint64 days = today.daysTo(event.dueDate);
            if (event.status != "Completed" && days >= 0 && days <= 3)
                lines << QString("%1 — %2 day(s) until due").arg(event.name).arg(days);
        }
        if (lines.isEmpty()) return;
        QMessageBox message(QMessageBox::Information, "Due in the next 3 days",
                            QString("%1 assignment(s) need your attention.").arg(lines.size()),
                            QMessageBox::Ok, this);
        message.setInformativeText(lines.mid(0, 10).join("\n")
                                   + (lines.size() > 10 ? "\nMore assignments in Show Details..." : ""));
        message.setTextFormat(Qt::PlainText);
        message.setDetailedText(lines.join("\n"));
        message.exec();
    });
}
