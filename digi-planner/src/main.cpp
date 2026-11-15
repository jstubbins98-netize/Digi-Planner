#include "Database.h"
#include "MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QFont>
#include <QLockFile>
#include <QMessageBox>
#include <QPalette>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTimer>
#include <exception>
#include <windows.h>
#include <winternl.h>

namespace {
bool isWindows11() {
    // Windows 11 reports major version 10. RtlGetVersion avoids compatibility-version lies.
    using GetVersion = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);
    const HMODULE module = GetModuleHandleW(L"ntdll.dll");
    if (!module) return false;
    const auto getVersion = reinterpret_cast<GetVersion>(GetProcAddress(module, "RtlGetVersion"));
    if (!getVersion) return false;
    RTL_OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    if (getVersion(&version) != 0 || version.dwMajorVersion != 10 || version.dwBuildNumber < 22000)
        return false;
    // Reject Windows Server even when its build number is newer than 22000.
    OSVERSIONINFOEXW workstation{};
    workstation.dwOSVersionInfoSize = sizeof(workstation);
    workstation.wProductType = VER_NT_WORKSTATION;
    const DWORDLONG mask = VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL);
    return VerifyVersionInfoW(&workstation, VER_PRODUCT_TYPE, mask) != FALSE;
}
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("DigiPlanner");
    QApplication::setApplicationName("DigiPlanner");
    QApplication::setApplicationVersion("1.0.0");
    if (!isWindows11()) {
        QMessageBox::critical(nullptr, "Windows 11 required", "Digi-Planner requires Windows 11.");
        return 1;
    }
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#eef4f2"));
    palette.setColor(QPalette::WindowText, QColor("#163c34"));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor("#f1f6f4"));
    palette.setColor(QPalette::Text, QColor("#173d35"));
    palette.setColor(QPalette::Button, QColor("#e3eeea"));
    palette.setColor(QPalette::ButtonText, QColor("#173d35"));
    palette.setColor(QPalette::Highlight, QColor("#c7e3d9"));
    palette.setColor(QPalette::HighlightedText, QColor("#102e26"));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, QColor("#173d35"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#7c8d87"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#7c8d87"));
    app.setPalette(palette);
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(
        "QLabel#heading {font-size:20px; font-weight:600; padding-bottom:6px;}"
        "QGroupBox {font-weight:600; border:1px solid #c8d8d2; border-radius:7px;"
        "margin-top:10px; padding:12px 8px 6px;}"
        "QGroupBox::title {subcontrol-origin:margin; left:12px; padding:0 5px;}"
        "QPushButton {padding:7px 12px;} "
        "QPushButton#primary {background:#17634f; color:white; border:0; border-radius:4px;}"
        "QPushButton#primary:hover {background:#124b3d;}"
        "QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox {padding:5px;}"
        "QTableWidget {border:1px solid #c8d8d2;}"
        "QHeaderView::section {padding:8px; background:#e3eeea; border:0;}"
    );
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        QMessageBox::critical(nullptr, "Cannot create data folder", "Check your Windows user-folder permissions.");
        return 1;
    }
    QLockFile lock(directory + "/planner.lock");
    if (!lock.tryLock(100)) {
        QMessageBox::warning(nullptr, "Cannot open Digi-Planner",
                             "Digi-Planner is already running, or its data folder cannot be locked.");
        return 1;
    }
    try {
        const QString path = directory + "/planner.db";
        Database database(path);
        MainWindow window(database, path);
        window.show();
        QTimer::singleShot(300, &window, &MainWindow::showUpcoming);
        return app.exec();
    } catch (const std::exception &error) {
        QMessageBox::critical(nullptr, "Digi-Planner could not start", QString::fromUtf8(error.what()));
        return 1;
    }
}
