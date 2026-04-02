#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QAbstractSocket>
#include <QTimer>
#include <QMutex>

/**
 * @brief WebSocket 长连接：自动重连、心跳、状态广播 JSON 发送
 */
class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);

    void setServerUrl(const QString &wsUrl);
    QString serverUrl() const { return m_url; }

    void setAuthToken(const QString &token);
    void connectToServer();
    void disconnectServer();

    bool isConnected() const;

    /** 发送聊天消息 JSON */
    void sendChatMessage(const QString &consultationId, const QString &text, const QString &clientMsgId = QString());
    /** 广播医生在线状态：online | busy | offline */
    void sendDoctorStatus(const QString &status);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &obj);
    void errorOccurred(const QString &err);
    void reconnecting(int attempt);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessage(const QString &msg);
    void onError(QAbstractSocket::SocketError error);
    void onReconnectTimeout();
    void onPingTimeout();

private:
    void scheduleReconnect();

    QWebSocket m_socket;
    QTimer m_reconnectTimer;
    QTimer m_pingTimer;
    QString m_url;
    QString m_token;
    mutable QMutex m_mutex;
    int m_reconnectAttempt = 0;
    static constexpr int kMaxReconnectMs = 30000;
};

#endif // WEBSOCKETCLIENT_H
