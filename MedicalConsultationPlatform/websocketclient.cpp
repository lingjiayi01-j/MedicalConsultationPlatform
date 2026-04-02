#include "websocketclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

WebSocketClient::WebSocketClient(QObject *parent) : QObject(parent)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::onReconnectTimeout);

    m_pingTimer.setInterval(25000);
    connect(&m_pingTimer, &QTimer::timeout, this, &WebSocketClient::onPingTimeout);

    connect(&m_socket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessage);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(&m_socket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);
#else
    typedef void (QAbstractSocket::*SocketErrSig)(QAbstractSocket::SocketError);
    connect(&m_socket, static_cast<SocketErrSig>(&QAbstractSocket::error), this, &WebSocketClient::onError);
#endif
}

void WebSocketClient::setServerUrl(const QString &wsUrl)
{
    QMutexLocker locker(&m_mutex);
    m_url = wsUrl.trimmed();
}

void WebSocketClient::setAuthToken(const QString &token)
{
    QMutexLocker locker(&m_mutex);
    m_token = token;
}

void WebSocketClient::connectToServer()
{
    QString urlStr;
    QString token;
    {
        QMutexLocker locker(&m_mutex);
        urlStr = m_url;
        token = m_token;
    }
    if (urlStr.isEmpty())
        return;

    QUrl url(urlStr);
    QUrlQuery q;
    if (!token.isEmpty()) {
        q.addQueryItem(QStringLiteral("token"), token);
        url.setQuery(q);
    }

    m_socket.open(url);
}

void WebSocketClient::disconnectServer()
{
    m_reconnectTimer.stop();
    m_pingTimer.stop();
    m_socket.close();
}

bool WebSocketClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::sendChatMessage(const QString &consultationId, const QString &text,
                                      const QString &clientMsgId)
{
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("chat"));
    o.insert(QStringLiteral("consultationId"), consultationId);
    o.insert(QStringLiteral("content"), text);
    o.insert(QStringLiteral("sender"), QStringLiteral("doctor"));
    if (!clientMsgId.isEmpty())
        o.insert(QStringLiteral("clientMsgId"), clientMsgId);
    const QByteArray payload = QJsonDocument(o).toJson(QJsonDocument::Compact);
    m_socket.sendTextMessage(QString::fromUtf8(payload));
}

void WebSocketClient::sendDoctorStatus(const QString &status)
{
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("doctor_status"));
    o.insert(QStringLiteral("status"), status);
    const QByteArray payload = QJsonDocument(o).toJson(QJsonDocument::Compact);
    if (isConnected())
        m_socket.sendTextMessage(QString::fromUtf8(payload));
}

void WebSocketClient::onConnected()
{
    m_reconnectAttempt = 0;
    m_pingTimer.start();
    emit connected();
}

void WebSocketClient::onDisconnected()
{
    m_pingTimer.stop();
    emit disconnected();
    scheduleReconnect();
}

void WebSocketClient::onTextMessage(const QString &msg)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError && doc.isObject())
        emit messageReceived(doc.object());
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit errorOccurred(m_socket.errorString());
    scheduleReconnect();
}

void WebSocketClient::onReconnectTimeout()
{
    ++m_reconnectAttempt;
    emit reconnecting(m_reconnectAttempt);
    connectToServer();
}

void WebSocketClient::onPingTimeout()
{
    if (!isConnected())
        return;
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("ping"));
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
}

void WebSocketClient::scheduleReconnect()
{
    if (m_reconnectTimer.isActive())
        return;
    const int delay = qMin(1000 * (1 << qMin(m_reconnectAttempt, 5)), kMaxReconnectMs);
    m_reconnectTimer.start(delay);
}
