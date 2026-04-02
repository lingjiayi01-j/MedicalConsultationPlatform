#ifndef LOCALDATABASE_H
#define LOCALDATABASE_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QMutex>

/**
 * @brief 本地 SQLite 缓存：会话列表、消息（离线可读）
 */
class LocalDatabase : public QObject
{
public:
    explicit LocalDatabase(QObject *parent = nullptr);
    ~LocalDatabase() override;

    bool init();

    void cacheConsultations(const QJsonArray &items);
    void cacheMessages(const QString &consultationId, const QJsonArray &messages);
    QJsonArray loadCachedConsultations(const QString &statusFilter = QString());
    QJsonArray loadCachedMessages(const QString &consultationId);

    void clearConsultationCache();

private:
    QString connectionName() const;
    QMutex m_mutex;
    bool m_ready = false;
};

#endif // LOCALDATABASE_H
