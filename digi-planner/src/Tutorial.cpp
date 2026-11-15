#include "Tutorial.h"
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

namespace {
struct Step {
    const char *title;
    const char *body;
};

const Step steps[] = {
    {"Welcome to Digi-Planner",
     "<p>This optional guide explains how to organize your assignments. "
     "It does not add, edit or delete any planner data.</p>"
     "<p>Use <b>Next</b> and <b>Back</b> to move through the guide. "
     "Choose <b>Close tutorial</b> or press <b>Esc</b> whenever you want to stop.</p>"
     "<p>You can keep this window open while trying the steps in the planner. "
     "Any changes you make in the planner itself are real and saved normally.</p>"
     "<p>To start the guide again, choose <b>Help &gt; Tutorial</b>, "
     "click <b>Tutorial</b> at the top of the planner, or press <b>F1</b>.</p>"},
    {"1. Add your classes",
     "<p>If you are in Calendar View, click <b>Switch to List View</b>.</p>"
     "<ol><li>Find <b>Class Management</b>.</li>"
     "<li>Type a class name into <b>New Class</b>.</li>"
     "<li>Click <b>Add</b>. The class becomes available in the class menus.</li></ol>"
     "<p><b>Remove Classes...</b> lets you remove classes after confirmation. "
     "Their assignments move to <b>Other</b> rather than being deleted. "
     "The Other class cannot be removed.</p>"},
    {"2. Create an assignment",
     "<p>In <b>Add New Event</b>, enter an <b>Event Name</b>, choose the "
     "<b>Due Date</b>, and select its <b>Status</b>, <b>Priority</b> and <b>Class</b>.</p>"
     "<p>Use <b>Time Estimate (h)</b> for the estimated work: for example, "
     "<b>1.5</b> means one hour and thirty minutes.</p>"
     "<p>Click <b>Add Event</b> to save the assignment.</p>"
     "<p><b>Recurring Weekly (7 weeks)</b> creates <b>8 independent assignments</b>: "
     "the chosen date plus seven weekly occurrences. Leave it unchecked for a one-time task.</p>"},
    {"3. Find and organize your work",
     "<p>Use <b>Search assignments...</b> to search by assignment name. "
     "Narrow the results with the <b>Class</b> and <b>Status</b> filters. "
     "<b>Clear Filters</b> restores the full list.</p>"
     "<p>Click a table column heading to sort; click again to reverse the order.</p>"
     "<p>The status dots and priority stars help distinguish tasks. "
     "Text labels and the legend explain each color.</p>"
     "<p><b>Total pending work</b> adds the estimated hours for visible assignments "
     "that are not Completed. Filters change this total.</p>"},
    {"4. Update or delete an assignment",
     "<p>Select an assignment row in the table. Use the controls beneath the table "
     "to change its <b>Class</b>, <b>Status</b> or <b>Priority</b>.</p>"
     "<p><b>Changes save immediately</b>; there is no separate Save button. "
     "Choose <b>Completed</b> when the work is finished. A task may disappear from "
     "the current filtered list after its status or class changes.</p>"
     "<p>The name, due date and time estimate are set when you create the assignment; "
     "these fields cannot currently be edited.</p>"
     "<p>Deleting requires confirmation and <b>cannot be undone</b>. "
     "Editing or deleting a recurring occurrence affects only that entry.</p>"},
    {"5. Check your calendar and reminders",
     "<p>Click <b>Switch to Calendar View</b> to see assignments by date. "
     "Use the month navigation to look ahead.</p>"
     "<p>Dates with assignments have dots. Click a date to see its assignments. "
     "Return with <b>Switch to List View</b> to add or update work.</p>"
     "<p>When the app starts, it shows a reminder if unfinished assignments are "
     "due today or within the next three days. This is a startup reminder, "
     "not a background notification service.</p>"},
    {"6. Export and keep your work",
     "<p>The export buttons at the bottom of List View let you save a "
     "<b>TXT</b> or <b>PDF</b> copy. Choose a filename and save location.</p>"
     "<p>Exports include <b>all assignments</b>, even when the list is filtered. "
     "They are readable copies, not backups that can be imported into the app.</p>"
     "<p>Your planner saves locally in a per-user SQLite database. "
     "The data location is shown at the bottom of List View.</p>"
     "<p>You are ready to use Digi-Planner. Click <b>Finish</b> to close this guide. "
     "You can reopen it anytime through <b>Help &gt; Tutorial</b> or <b>F1</b>.</p>"}
};
}

void Tutorial::show(QWidget *parent) {
    // Keep just one modeless guide, so the planner remains available for practice.
    if (auto *existing = parent->findChild<QWizard *>("plannerTutorial")) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
    }
    auto *wizard = new QWizard(parent);
    wizard->setObjectName("plannerTutorial");
    wizard->setAttribute(Qt::WA_DeleteOnClose);
    wizard->setWindowTitle("Digi-Planner Tutorial");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->setOption(QWizard::NoBackButtonOnStartPage);
    wizard->setOption(QWizard::NoCancelButtonOnLastPage, false);
    wizard->setButtonText(QWizard::CancelButton, "Close tutorial");
    wizard->resize(640, 480);
    int index = 0;
    for (const auto &step : steps) {
        auto *page = new QWizardPage(wizard);
        page->setTitle(QString::fromUtf8(step.title));
        page->setSubTitle(QString("Step %1 of 7").arg(++index));
        auto *layout = new QVBoxLayout(page);
        auto *body = new QTextBrowser(page);
        body->setAccessibleName(QString::fromUtf8(step.title));
        body->setHtml(QString::fromUtf8(step.body));
        layout->addWidget(body);
        wizard->addPage(page);
    }
    wizard->show();
}
