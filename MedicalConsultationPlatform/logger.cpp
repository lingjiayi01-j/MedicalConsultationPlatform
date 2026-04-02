#include "logger.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QtGlobal>

static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QString level;
    switch (type) {
    case QtDebugMsg: level = QStringLiteral("DEBUG"); break;
    case QtWarningMsg: level = QStringLiteral("WARN"); break;
    case QtCriticalMsg: level = QStringLiteral("CRIT"); break;
    case QtFatalMsg: level = QStringLiteral("FATAL"); break;
    case QtInfoMsg: level = QStringLiteral("INFO"); break;
    default: level = QStringLiteral("LOG"); break;
    }
    Logger::instance().log(level, msg);
}

Logger &Logger::instance()
{
    static Logger s;
    return s;
}

void Logger::init(const QString &logDir)
{
    QMutexLocker locker(&m_mutex);
    if (m_installed)
        return;

    QString dir = logDir;
    if (dir.isEmpty())
        dir = QCoreApplication::applicationDirPath() + QStringLiteral("/logs");

    QDir().mkpath(dir);
    m_filePath = dir + QStringLiteral("/doctor_%1.log")
                     .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));

    qInstallMessageHandler(qtMessageHandler);
    m_installed = true;
}

void Logger::log(const QString &level, const QString &message)
{
    QMutexLocker locker(&m_mutex);
    const QString line = QStringLiteral("[%1] [%2] %3\n")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                  level, message);

    if (!m_filePath.isEmpty()) {
        QFile f(m_filePath);
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&f);
            out << line;
        }
    }
}
