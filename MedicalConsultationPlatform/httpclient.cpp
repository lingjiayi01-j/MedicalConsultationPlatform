#include "httpclient.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QSslError>

HttpClient::HttpClient(QObject *parent) : QObject(parent)
{
    connect(&m_nam, &QNetworkAccessManager::finished, this, &HttpClient::onReplyFinished);
}

void HttpClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url.trimmed();
    if (m_baseUrl.endsWith(QLatin1Char('/')))
        m_baseUrl.chop(1);
}

void HttpClient::setAuthToken(const QString &token)
{
    QMutexLocker locker(&m_tokenMutex);
    m_token = token;
}

QString HttpClient::authToken() const
{
    QMutexLocker locker(&m_tokenMutex);
    return m_token;
}

QUrl HttpClient::buildUrl(const QString &path) const
{
    QString p = path;
    if (!p.startsWith(QLatin1Char('/')))
        p.prepend(QLatin1Char('/'));
    return QUrl(m_baseUrl + p);
}

void HttpClient::postJson(const QString &path, const QJsonObject &json, JsonCallback cb)
{
    QNetworkRequest req;
    req.setUrl(buildUrl(path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QString tok = authToken();
    if (!tok.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + tok.toUtf8());
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);

    QJsonDocument doc(json);
    QNetworkReply *reply = m_nam.post(req, doc.toJson(QJsonDocument::Compact));
    m_jsonCallbacks.insert(reply, cb);
    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [](const QList<QSslError> &e) {
        Q_UNUSED(e);
        auto *r = qobject_cast<QNetworkReply *>(sender());
        if (r)
            r->ignoreSslErrors();
    });
}

void HttpClient::getJson(const QString &path, JsonCallback cb)
{
    QNetworkRequest req;
    req.setUrl(buildUrl(path));
    const QString tok = authToken();
    if (!tok.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + tok.toUtf8());
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    QNetworkReply *reply = m_nam.get(req);
    m_jsonCallbacks.insert(reply, cb);
    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [](const QList<QSslError> &e) {
        Q_UNUSED(e);
        auto *r = qobject_cast<QNetworkReply *>(sender());
        if (r)
            r->ignoreSslErrors();
    });
}

void HttpClient::uploadFile(const QString &path, const QString &fieldName, const QByteArray &fileData,
                            const QString &fileName, const QJsonObject &extraFields, JsonCallback cb)
{
    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
                                    .arg(fieldName, fileName)));
    filePart.setBody(fileData);
    multi->append(filePart);

    for (auto it = extraFields.begin(); it != extraFields.end(); ++it) {
        QHttpPart p;
        p.setHeader(QNetworkRequest::ContentDispositionHeader,
                    QVariant(QStringLiteral("form-data; name=\"%1\"").arg(it.key())));
        p.setBody(it.value().toString().toUtf8());
        multi->append(p);
    }

    QNetworkRequest req;
    req.setUrl(buildUrl(path));
    const QString tok = authToken();
    if (!tok.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + tok.toUtf8());

    QNetworkReply *reply = m_nam.post(req, multi);
    multi->setParent(reply);
    m_jsonCallbacks.insert(reply, cb);
    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [](const QList<QSslError> &e) {
        Q_UNUSED(e);
        auto *r = qobject_cast<QNetworkReply *>(sender());
        if (r)
            r->ignoreSslErrors();
    });
}

void HttpClient::onReplyFinished(QNetworkReply *reply)
{
    if (!reply)
        return;

    const JsonCallback cb = m_jsonCallbacks.take(reply);
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 401)
        emit unauthorized();

    const QByteArray data = reply->readAll();
    QString errStr;
    if (reply->error() != QNetworkReply::NoError)
        errStr = reply->errorString();

    reply->deleteLater();

    if (!cb)
        return;

    QJsonObject obj;
    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &perr);
    if (perr.error == QJsonParseError::NoError && doc.isObject())
        obj = doc.object();
    else if (!data.isEmpty())
        obj.insert(QStringLiteral("raw"), QString::fromUtf8(data));

    cb(status, obj, errStr);
}
