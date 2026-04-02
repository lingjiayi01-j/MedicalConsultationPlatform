/**
 * @file main.cpp
 * @brief 医疗咨询平台（医生端）入口：日志、统一样式、登录、主窗口、托盘生命周期
 */
#include <QApplication>
#include <QStyleFactory>
#include <QSettings>
#include "appconfig.h"
#include "logger.h"
#include "httpclient.h"
#include "logindialog.h"
#include "mainwindow.h"

/** 医疗场景：清爽蓝绿色调全局 QSS */
static QString medicalStyleSheet()
{
    return QStringLiteral(
        "QMainWindow, QDialog { background: #f6fafb; }"
        "QMenuBar { background: #e8f4fc; border-bottom: 1px solid #cfe2ff; }"
        "QMenuBar::item:selected { background: #cfe2ff; }"
        "QStatusBar { background: #eef7f4; border-top: 1px solid #c5e1d5; }"
        "QPushButton {"
        "  background-color: #0d6efd;"
        "  color: white;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  border: none;"
        "}"
        "QPushButton:hover { background-color: #0b5ed7; }"
        "QPushButton:pressed { background-color: #0a58ca; }"
        "QPushButton:disabled { background-color: #9fbce3; }"
        "QLineEdit, QComboBox, QSpinBox, QTimeEdit {"
        "  border: 1px solid #cfe2ff;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "  background: white;"
        "}"
        "QTabWidget::pane { border: 1px solid #cfe2ff; background: white; }"
        "QTabBar::tab:selected { background: #e8f4fc; }"
        "QListWidget { background: white; border: 1px solid #cfe2ff; }"
        "QGroupBox { font-weight: 600; border: 1px solid #cfe2ff; border-radius: 6px; margin-top: 8px; }"
        "QTextBrowser { border: 1px solid #cfe2ff; background: white; border-radius: 6px; }");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(AppConfig::organizationName());
    QCoreApplication::setApplicationName(AppConfig::applicationName());

    Logger::instance().init();
    qInfo() << QStringLiteral("应用程序启动");

    if (QStyleFactory::keys().contains(QStringLiteral("Fusion")))
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setStyleSheet(medicalStyleSheet());

    QSettings settings(AppConfig::organizationName(), AppConfig::applicationName());
    HttpClient *http = new HttpClient();
    http->setBaseUrl(settings.value(QStringLiteral("api/baseUrl"), AppConfig::defaultApiBaseUrl()).toString());

    LoginDialog login(http);
    if (login.exec() != QDialog::Accepted) {
        delete http;
        return 0;
    }

    MainWindow mainWin(http);
    mainWin.applyAuth(login.authToken(), login.doctorName(), login.doctorId());
    mainWin.show();

    return app.exec(); // http 由 MainWindow 托管释放
}
