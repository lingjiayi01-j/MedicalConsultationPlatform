#include "cryptohelper.h"
#include <QByteArray>
#include <QCryptographicHash>

namespace CryptoHelper {

static QByteArray deriveKey()
{
    // 与机器/应用绑定的派生密钥（演示）
    const QByteArray salt = QByteArrayLiteral("MCP-Doctor-2026-salt");
    return QCryptographicHash::hash(salt, QCryptographicHash::Sha256);
}

QString encryptPassword(const QString &plain)
{
    if (plain.isEmpty())
        return {};
    const QByteArray key = deriveKey();
    QByteArray data = plain.toUtf8();
    for (int i = 0; i < data.size(); ++i)
        data[i] = static_cast<char>(data[i] ^ key.at(i % key.size()));
    return QString::fromLatin1(data.toHex());
}

QString decryptPassword(const QString &cipherHex)
{
    if (cipherHex.isEmpty())
        return {};
    const QByteArray key = deriveKey();
    QByteArray data = QByteArray::fromHex(cipherHex.toLatin1());
    for (int i = 0; i < data.size(); ++i)
        data[i] = static_cast<char>(data[i] ^ key.at(i % key.size()));
    return QString::fromUtf8(data);
}

} // namespace CryptoHelper
