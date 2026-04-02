#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

/**
 * @file appconfig.h
 * @brief 全局 API / WebSocket 地址与组织名（可通过 QSettings 覆盖）
 */
namespace AppConfig {
inline QString organizationName() { return QStringLiteral("MedicalConsultationPlatform"); }
inline QString applicationName() { return QStringLiteral("DoctorClient"); }

// 默认后端地址（可在设置中修改并持久化）
inline QString defaultApiBaseUrl() { return QStringLiteral("http://127.0.0.1:8080"); }
inline QString defaultWsBaseUrl() { return QStringLiteral("ws://127.0.0.1:8080/ws"); }
}

#endif // APPCONFIG_H
