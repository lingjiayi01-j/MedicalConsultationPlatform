#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QJsonObject>
#include <QString>
#include <QHash>
#include <QTimer>

class QLabel;
class QTextBrowser;
class QLineEdit;
class QPushButton;
class HttpClient;
class WebSocketClient;
class LocalDatabase;

/**
 * @brief 会话聊天：历史拉取、气泡展示、WebSocket 发送、附件上传、重发、结束咨询、处方入口
 */
class ChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChatWidget(HttpClient *http, WebSocketClient *ws, LocalDatabase *cache, QWidget *parent = nullptr);

    QString consultationId() const { return m_consultationId; }
    void setConsultation(const QString &id, const QJsonObject &patientMeta);

    /** 来自主窗口转发的 WS 消息 */
    void handleIncomingJson(const QJsonObject &obj);

    void markRead();

signals:
    void consultationClosed(const QString &id);
    void flashRequested();

private slots:
    void onSend();
    void onAttach();
    void onEnd();
    void onPrescription();
    void tryResendPending();

private:
    void fetchHistory();
    void appendBubble(bool fromDoctor, const QString &text, const QString &attachmentUrl = QString());
    void scrollToBottom();
    QString bubbleHtml(bool fromDoctor, const QString &text, const QString &att) const;

    HttpClient *m_http = nullptr;
    WebSocketClient *m_ws = nullptr;
    LocalDatabase *m_cache = nullptr;

    QString m_consultationId;
    QJsonObject m_patient;

    QLabel *m_infoLabel = nullptr;
    QTextBrowser *m_view = nullptr;
    QLineEdit *m_input = nullptr;

    struct PendingMsg {
        QString clientId;
        QString text;
        int retries = 0;
    };
    QHash<QString, PendingMsg> m_pending;
    QTimer m_resendTimer;
};

#endif // CHATWIDGET_H
