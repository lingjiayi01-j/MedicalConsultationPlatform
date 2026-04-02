#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QMutex>
#include <functional>
#include <QHash>

/**
 * @brief 基于 QNetworkAccessManager 的 REST 封装（在同一线程内完成请求）
 */
class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    QString baseUrl() const { return m_baseUrl; }
    void setAuthToken(const QString &token);
    QString authToken() const;

    using JsonCallback = std::function<void(int httpStatus, const QJsonObject &body, const QString &error)>;

    void postJson(const QString &path, const QJsonObject &json, JsonCallback cb);
    void getJson(const QString &path, JsonCallback cb);
    void uploadFile(const QString &path, const QString &fieldName, const QByteArray &fileData,
                    const QString &fileName, const QJsonObject &extraFields, JsonCallback cb);

signals:
    void unauthorized();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QUrl buildUrl(const QString &path) const;

    QNetworkAccessManager m_nam;
    QString m_baseUrl;
    QString m_token;
    mutable QMutex m_tokenMutex;
    QHash<QNetworkReply *, JsonCallback> m_jsonCallbacks;
};

#endif // HTTPCLIENT_H
