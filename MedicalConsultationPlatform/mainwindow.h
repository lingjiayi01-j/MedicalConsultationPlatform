#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <QString>
#include <QCloseEvent>
#include <QJsonObject>

class QListWidget;
class QStackedWidget;
class QComboBox;
class QLabel;
class QAction;
class QTabWidget;
class QSystemTrayIcon;
class HttpClient;
class WebSocketClient;
class QListWidgetItem;
class LocalDatabase;
class ChatWidget;

/**
 * @brief 医生主界面：患者分栏、聊天堆栈、WebSocket、托盘、未读与状态
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(HttpClient *httpClient, QWidget *parent = nullptr);
    ~MainWindow() override;

    void applyAuth(const QString &token, const QString &doctorName, const QString &doctorId);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onOpenSchedule();
    void onOpenEarnings();
    void onOpenSettings();
    void onShowHelp();
    void onExit();
    void onTrayActivate(QSystemTrayIcon::ActivationReason reason);
    void onWsMessage(const QJsonObject &obj);
    void onUnauthorized();
    void onPatientClicked(QListWidgetItem *item);
    void refreshPatientLists();
    void onStatusChanged(int index);
    void onChatFlash();
    void onChatClosed(const QString &cid);

private:
    void setupMenus();
    void setupUi();
    void setupTray();
    void setupConnections();
    ChatWidget *ensureChat(const QString &consultationId, const QJsonObject &patient);
    void bumpUnread(int d);
    void flashWindow();

    HttpClient *m_http = nullptr;
    WebSocketClient *m_ws = nullptr;
    LocalDatabase *m_db = nullptr;

    QString m_doctorName;
    QString m_doctorId;

    QTabWidget *m_tabs = nullptr;
    QListWidget *m_listPending = nullptr;
    QListWidget *m_listActive = nullptr;
    QListWidget *m_listDone = nullptr;

    QStackedWidget *m_stack = nullptr;
    QHash<QString, ChatWidget *> m_chats;

    QComboBox *m_statusCombo = nullptr;
    QLabel *m_unreadLabel = nullptr;
    QLabel *m_nameLabel = nullptr;

    QSystemTrayIcon *m_tray = nullptr;
    QAction *m_trayShow = nullptr;

    int m_unread = 0;
    bool m_quitting = false;
};

#endif // MAINWINDOW_H
