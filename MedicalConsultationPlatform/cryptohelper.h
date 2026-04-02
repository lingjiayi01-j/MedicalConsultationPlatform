#ifndef CRYPTOHELPER_H
#define CRYPTOHELPER_H

#include <QString>
#include <QByteArray>

/**
 * @brief 简单对称混淆存储（演示用）；生产环境请使用平台钥匙串或国密/标准算法库
 */
namespace CryptoHelper {
QString encryptPassword(const QString &plain);
QString decryptPassword(const QString &cipherHex);
}

#endif // CRYPTOHELPER_H
