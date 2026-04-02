#include "mainwindow.h"
#include "httpclient.h"
#include "websocketclient.h"
#include "localdatabase.h"
#include "chatwidget.h"
#include "scheduledialog.h"
#include "earningsdialog.h"
#include "settingsdialog.h"
#include "appconfig.h"
#include "logger.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QSplitter>
#include <QTabWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QApplication>
#include <QCloseEvent>
#include <QTimer>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidgetItem>


MainWindow::MainWindow(HttpClient *httpClient, QWidget *parent) : QMainWindow(parent)
{
    m_http = httpClient;
    if (m_http)
        m_http->setParent(this);

    m_ws = new WebSocketClient(this);
    m_db = new LocalDatabase(this);
    m_db->init();

    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    m_http->setBaseUrl(s.value(QStringLiteral("api/baseUrl"), AppConfig::defaultApiBaseUrl()).toString());

    setWindowTitle(QStringLiteral("医疗咨询平台 — 医生工作台"));
    resize(1280, 800);

    setupUi();
    setupMenus();
    setupTray();
    setupConnections();

    QTimer::singleShot(100, this, &MainWindow::refreshPatientLists);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupMenus()
{
    QMenu *work = menuBar()->addMenu(QStringLiteral("工作台"));
    work->addAction(QStringLiteral("刷新患者列表"), this, &MainWindow::refreshPatientLists);

    QMenu *sch = menuBar()->addMenu(QStringLiteral("排班"));
    sch->addAction(QStringLiteral("排班设置…"), this, &MainWindow::onOpenSchedule);

    QMenu *earn = menuBar()->addMenu(QStringLiteral("收益"));
    earn->addAction(QStringLiteral("收益统计…"), this, &MainWindow::onOpenEarnings);

    QMenu *set = menuBar()->addMenu(QStringLiteral("设置"));
    set->addAction(QStringLiteral("个人设置…"), this, &MainWindow::onOpenSettings);

    QMenu *help = menuBar()->addMenu(QStringLiteral("帮助"));
    help->addAction(QStringLiteral("关于"), this, &MainWindow::onShowHelp);

    menuBar()->addAction(QStringLiteral("退出"), this, &MainWindow::onExit);
}

void MainWindow::setupUi()
{
    auto *split = new QSplitter(Qt::Horizontal, this);

    m_tabs = new QTabWidget(this);
    m_listPending = new QListWidget(this);
    m_listActive = new QListWidget(this);
    m_listDone = new QListWidget(this);
    m_tabs->addTab(m_listPending, QStringLiteral("待接诊"));
    m_tabs->addTab(m_listActive, QStringLiteral("进行中"));
    m_tabs->addTab(m_listDone, QStringLiteral("已完成"));

    m_stack = new QStackedWidget(this);
    auto *placeholder = new QLabel(QStringLiteral("请选择左侧患者开始问诊"), this);
    placeholder->setAlignment(Qt::AlignCenter);
    m_stack->addWidget(placeholder);

    split->addWidget(m_tabs);
    split->addWidget(m_stack);
    split->setStretchFactor(1, 3);
    setCentralWidget(split);

    m_nameLabel = new QLabel(QStringLiteral("医生：—"), this);
    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItems({QStringLiteral("在线"), QStringLiteral("忙碌"), QStringLiteral("离线")});
    m_unreadLabel = new QLabel(QStringLiteral("未读：0"), this);

    statusBar()->addPermanentWidget(m_nameLabel);
    statusBar()->addPermanentWidget(new QLabel(QStringLiteral("状态"), this));
    statusBar()->addPermanentWidget(m_statusCombo);
    statusBar()->addPermanentWidget(m_unreadLabel);

    connect(m_listPending, &QListWidget::itemClicked, this, &MainWindow::onPatientClicked);
    connect(m_listActive, &QListWidget::itemClicked, this, &MainWindow::onPatientClicked);
    connect(m_listDone, &QListWidget::itemClicked, this, &MainWindow::onPatientClicked);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onStatusChanged);
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    m_tray = new QSystemTrayIcon(QIcon::fromTheme(QStringLiteral("dialog-information")), this);
    m_trayShow = new QAction(QStringLiteral("显示主窗口"), this);
    auto *quit = new QAction(QStringLiteral("退出"), this);
    QMenu *m = new QMenu;
    m->addAction(m_trayShow);
    m->addSeparator();
    m->addAction(quit);
    m_tray->setContextMenu(m);
    m_tray->setToolTip(QStringLiteral("医疗咨询 — 医生端"));

    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivate);
    connect(m_trayShow, &QAction::triggered, this, &QWidget::showNormal);
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->show();
}

void MainWindow::setupConnections()
{
    connect(m_ws, &WebSocketClient::messageReceived, this, &MainWindow::onWsMessage);
    connect(m_http, &HttpClient::unauthorized, this, &MainWindow::onUnauthorized);
}

void MainWindow::applyAuth(const QString &token, const QString &doctorName, const QString &doctorId)
{
    m_doctorName = doctorName;
    m_doctorId = doctorId;
    m_http->setAuthToken(token);
    m_ws->setAuthToken(token);

    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    const QString wsUrl = s.value(QStringLiteral("api/wsUrl"), AppConfig::defaultWsBaseUrl()).toString();
    m_ws->setServerUrl(wsUrl);
    m_ws->connectToServer();

    m_nameLabel->setText(QStringLiteral("医生：%1").arg(m_doctorName.isEmpty() ? m_doctorId : m_doctorName));

    const QString st = m_statusCombo->currentText();
    if (st == QStringLiteral("在线"))
        m_ws->sendDoctorStatus(QStringLiteral("online"));
    else if (st == QStringLiteral("忙碌"))
        m_ws->sendDoctorStatus(QStringLiteral("busy"));
    else
        m_ws->sendDoctorStatus(QStringLiteral("offline"));

    refreshPatientLists();
    Logger::instance().info(QStringLiteral("登录成功，医生：%1").arg(m_doctorName));
}

void MainWindow::refreshPatientLists()
{
    m_http->getJson(QStringLiteral("/api/consultations/list"), [this](int status, const QJsonObject &resp, const QString &) {
        QJsonArray items;
        if (status == 200) {
            items = resp.value(QStringLiteral("data")).toObject().value(QStringLiteral("items")).toArray();
            if (items.isEmpty())
                items = resp.value(QStringLiteral("items")).toArray();
        }
        if (items.isEmpty() && m_db)
            items = m_db->loadCachedConsultations(QString());

        if (!items.isEmpty() && m_db)
            m_db->cacheConsultations(items);

        auto fill = [](QListWidget *lw, const QJsonArray &all, const QString &st) {
            lw->clear();
            for (const auto &v : all) {
                const QJsonObject o = v.toObject();
                if (o.value(QStringLiteral("status")).toString() != st)
                    continue;
                const QString id = o.value(QStringLiteral("id")).toString();
                const QString name = o.value(QStringLiteral("patientName")).toString();
                auto *it = new QListWidgetItem(QStringLiteral("%1  (%2)").arg(name, id));
                it->setData(Qt::UserRole, id);
                it->setData(Qt::UserRole + 1, QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
                lw->addItem(it);
            }
        };

        fill(m_listPending, items, QStringLiteral("pending"));
        fill(m_listActive, items, QStringLiteral("active"));
        fill(m_listDone, items, QStringLiteral("closed"));
    });
}

ChatWidget *MainWindow::ensureChat(const QString &consultationId, const QJsonObject &patient)
{
    if (m_chats.contains(consultationId))
        return m_chats.value(consultationId);

    auto *cw = new ChatWidget(m_http, m_ws, m_db, this);
    connect(cw, &ChatWidget::flashRequested, this, &MainWindow::onChatFlash);
    connect(cw, &ChatWidget::consultationClosed, this, &MainWindow::onChatClosed);

    cw->setConsultation(consultationId, patient);
    m_stack->addWidget(cw);
    m_chats.insert(consultationId, cw);
    return cw;
}

void MainWindow::onPatientClicked(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString id = item->data(Qt::UserRole).toString();
    QJsonParseError err{};
    const QJsonDocument doc =
        QJsonDocument::fromJson(item->data(Qt::UserRole + 1).toString().toUtf8(), &err);
    const QJsonObject patient = (err.error == QJsonParseError::NoError && doc.isObject()) ? doc.object() : QJsonObject{};
    ChatWidget *cw = ensureChat(id, patient);
    m_stack->setCurrentWidget(cw);
    cw->markRead();
}

void MainWindow::onWsMessage(const QJsonObject &obj)
{
    const QString type = obj.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("chat") || type == QLatin1String("message")) {
        const QString cid = obj.value(QStringLiteral("consultationId")).toString();
        ChatWidget *cw = m_chats.value(cid, nullptr);
        if (cw)
            cw->handleIncomingJson(obj);

        ChatWidget *current = qobject_cast<ChatWidget *>(m_stack->currentWidget());
        if (!current || current->consultationId() != cid)
            bumpUnread(1);
    }
}

void MainWindow::bumpUnread(int d)
{
    m_unread = qMax(0, m_unread + d);
    m_unreadLabel->setText(QStringLiteral("未读：%1").arg(m_unread));
    if (d > 0) {
        flashWindow();
        if (m_tray)
            m_tray->showMessage(QStringLiteral("新消息"), QStringLiteral("您有新的患者消息"),
                                QSystemTrayIcon::Information, 3000);
    }
}

void MainWindow::flashWindow()
{
    QApplication::alert(this);
}

void MainWindow::onChatFlash()
{
    flashWindow();
}

void MainWindow::onChatClosed(const QString &cid)
{
    m_chats.remove(cid);
    refreshPatientLists();
    for (int i = m_stack->count() - 1; i >= 1; --i) {
        auto *c = qobject_cast<ChatWidget *>(m_stack->widget(i));
        if (c && c->consultationId() == cid) {
            m_stack->removeWidget(c);
            c->deleteLater();
            break;
        }
    }
    if (qobject_cast<ChatWidget *>(m_stack->currentWidget()) == nullptr)
        m_stack->setCurrentIndex(0);
}

void MainWindow::onStatusChanged(int index)
{
    Q_UNUSED(index);
    const QString text = m_statusCombo->currentText();
    QString code = QStringLiteral("offline");
    if (text == QStringLiteral("在线"))
        code = QStringLiteral("online");
    else if (text == QStringLiteral("忙碌"))
        code = QStringLiteral("busy");
    m_ws->sendDoctorStatus(code);
    Logger::instance().info(QStringLiteral("状态切换：%1").arg(code));
}

void MainWindow::onUnauthorized()
{
    QMessageBox::warning(this, QStringLiteral("登录失效"), QStringLiteral("请重新登录"));
}

void MainWindow::onOpenSchedule()
{
    ScheduleDialog dlg(this);
    dlg.exec();
}

void MainWindow::onOpenEarnings()
{
    EarningsDialog dlg(m_http, this);
    dlg.exec();
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(m_http, this);
    dlg.exec();
    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    m_http->setBaseUrl(s.value(QStringLiteral("api/baseUrl"), AppConfig::defaultApiBaseUrl()).toString());
    const QString ws = s.value(QStringLiteral("api/wsUrl"), AppConfig::defaultWsBaseUrl()).toString();
    m_ws->disconnectServer();
    m_ws->setServerUrl(ws);
    m_ws->connectToServer();
}

void MainWindow::onShowHelp()
{
    QMessageBox::about(this, QStringLiteral("关于"),
                       QStringLiteral("医疗咨询平台 医生端 v1.0\n基于 Qt 6，演示用途。"));
}

void MainWindow::onExit()
{
    m_quitting = true;
    qApp->quit();
}

void MainWindow::onTrayActivate(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        showNormal();
        activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_quitting) {
        event->accept();
        return;
    }
    if (m_tray && m_tray->isVisible()) {
        hide();
        event->ignore();
        if (m_tray)
            m_tray->showMessage(QStringLiteral("仍在后台运行"), QStringLiteral("点击托盘图标可恢复窗口"),
                                QSystemTrayIcon::Information, 2000);
    } else {
        event->accept();
    }
}
