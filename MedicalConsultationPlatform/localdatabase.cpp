#include "localdatabase.h"
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>

LocalDatabase::LocalDatabase(QObject *parent) : QObject(parent) {}

LocalDatabase::~LocalDatabase()
{
    const QString name = connectionName();
    QSqlDatabase db = QSqlDatabase::database(name, false);
    if (db.isOpen())
        db.close();
    QSqlDatabase::removeDatabase(name);
}

QString LocalDatabase::connectionName() const
{
    return QStringLiteral("mcp_local_%1").arg(quintptr(QThread::currentThreadId()));
}

bool LocalDatabase::init()
{
    QMutexLocker locker(&m_mutex);
    if (m_ready)
        return true;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName());
    db.setDatabaseName(QCoreApplication::applicationDirPath() + QStringLiteral("/doctor_cache.sqlite"));

    if (!db.open()) {
        qWarning() << QStringLiteral("SQLite 打开失败:") << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS consultations ("
        "id TEXT PRIMARY KEY, status TEXT, patient_name TEXT, avatar TEXT, last_msg TEXT, updated_at TEXT, raw_json TEXT)"));
    q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, consultation_id TEXT, msg_id TEXT, sender TEXT, content TEXT, "
        "created_at TEXT, attachment TEXT, raw_json TEXT)"));
    q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_messages_cid ON messages(consultation_id)"));

    m_ready = true;
    return true;
}

void LocalDatabase::cacheConsultations(const QJsonArray &items)
{
    if (!init())
        return;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(connectionName());
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO consultations (id, status, patient_name, avatar, last_msg, updated_at, raw_json) "
        "VALUES (:id, :status, :patient_name, :avatar, :last_msg, :updated_at, :raw_json)"));

    for (const QJsonValue &v : items) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        q.bindValue(QStringLiteral(":id"), o.value(QStringLiteral("id")).toString());
        q.bindValue(QStringLiteral(":status"), o.value(QStringLiteral("status")).toString());
        q.bindValue(QStringLiteral(":patient_name"), o.value(QStringLiteral("patientName")).toString());
        q.bindValue(QStringLiteral(":avatar"), o.value(QStringLiteral("avatar")).toString());
        q.bindValue(QStringLiteral(":last_msg"), o.value(QStringLiteral("lastMessage")).toString());
        q.bindValue(QStringLiteral(":updated_at"), o.value(QStringLiteral("updatedAt")).toString());
        q.bindValue(QStringLiteral(":raw_json"), QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
        if (!q.exec())
            qWarning() << q.lastError().text();
    }
}

void LocalDatabase::cacheMessages(const QString &consultationId, const QJsonArray &messages)
{
    if (!init() || consultationId.isEmpty())
        return;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(connectionName());
    QSqlQuery del(db);
    del.prepare(QStringLiteral("DELETE FROM messages WHERE consultation_id = :cid"));
    del.bindValue(QStringLiteral(":cid"), consultationId);
    del.exec();

    QSqlQuery ins(db);
    ins.prepare(QStringLiteral(
        "INSERT INTO messages (consultation_id, msg_id, sender, content, created_at, attachment, raw_json) "
        "VALUES (:cid, :mid, :sender, :content, :created, :att, :raw)"));

    for (const QJsonValue &v : messages) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        ins.bindValue(QStringLiteral(":cid"), consultationId);
        ins.bindValue(QStringLiteral(":mid"), o.value(QStringLiteral("id")).toString());
        ins.bindValue(QStringLiteral(":sender"), o.value(QStringLiteral("sender")).toString());
        ins.bindValue(QStringLiteral(":content"), o.value(QStringLiteral("content")).toString());
        ins.bindValue(QStringLiteral(":created"), o.value(QStringLiteral("createdAt")).toString());
        ins.bindValue(QStringLiteral(":att"), o.value(QStringLiteral("attachmentUrl")).toString());
        ins.bindValue(QStringLiteral(":raw"), QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
        if (!ins.exec())
            qWarning() << ins.lastError().text();
    }
}

QJsonArray LocalDatabase::loadCachedConsultations(const QString &statusFilter)
{
    QJsonArray out;
    if (!init())
        return out;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(connectionName());
    QSqlQuery q(db);
    QString sql = QStringLiteral("SELECT raw_json FROM consultations");
    if (!statusFilter.isEmpty())
        sql += QStringLiteral(" WHERE status = :st");
    q.prepare(sql);
    if (!statusFilter.isEmpty())
        q.bindValue(QStringLiteral(":st"), statusFilter);
    if (!q.exec())
        return out;
    while (q.next()) {
        const QByteArray raw = q.value(0).toByteArray();
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            out.append(doc.object());
    }
    return out;
}

QJsonArray LocalDatabase::loadCachedMessages(const QString &consultationId)
{
    QJsonArray out;
    if (!init() || consultationId.isEmpty())
        return out;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(connectionName());
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT raw_json FROM messages WHERE consultation_id = :cid ORDER BY id ASC"));
    q.bindValue(QStringLiteral(":cid"), consultationId);
    if (!q.exec())
        return out;
    while (q.next()) {
        const QByteArray raw = q.value(0).toByteArray();
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            out.append(doc.object());
    }
    return out;
}

void LocalDatabase::clearConsultationCache()
{
    if (!init())
        return;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database(connectionName());
    QSqlQuery q(db);
    q.exec(QStringLiteral("DELETE FROM consultations"));
    q.exec(QStringLiteral("DELETE FROM messages"));
}
