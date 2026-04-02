#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QMutex>

/**
 * @brief 线程安全文件日志 + Qt 消息钩子
 */
class Logger
{
public:
    static Logger &instance();

    void init(const QString &logDir = QString());
    void log(const QString &level, const QString &message);

    void info(const QString &msg) { log(QStringLiteral("INFO"), msg); }
    void warning(const QString &msg) { log(QStringLiteral("WARN"), msg); }
    void error(const QString &msg) { log(QStringLiteral("ERROR"), msg); }

private:
    Logger() = default;
    Q_DISABLE_COPY(Logger)

    QMutex m_mutex;
    QString m_filePath;
    bool m_installed = false;
};

#endif // LOGGER_H
