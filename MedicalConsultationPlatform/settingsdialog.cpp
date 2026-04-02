#include "settingsdialog.h"
#include "httpclient.h"
#include "appconfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QFrame>
#include <QFile>
#include <QFileInfo>

SettingsDialog::SettingsDialog(HttpClient *http, QWidget *parent) : QDialog(parent), m_http(http)
{
    setWindowTitle(QStringLiteral("个人设置"));
    resize(480, 560);

    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());

    auto *profileBox = new QGroupBox(QStringLiteral("个人信息"), this);
    m_nameEdit = new QLineEdit(s.value(QStringLiteral("profile/name")).toString(), this);
    m_titleEdit = new QLineEdit(s.value(QStringLiteral("profile/title")).toString(), this);
    m_phoneEdit = new QLineEdit(s.value(QStringLiteral("profile/phone")).toString(), this);
    m_deptEdit = new QLineEdit(s.value(QStringLiteral("profile/dept")).toString(), this);
    auto *pfLay = new QFormLayout(profileBox);
    pfLay->addRow(QStringLiteral("姓名"), m_nameEdit);
    pfLay->addRow(QStringLiteral("职称"), m_titleEdit);
    pfLay->addRow(QStringLiteral("手机"), m_phoneEdit);
    pfLay->addRow(QStringLiteral("科室"), m_deptEdit);
    m_apiUrlEdit = new QLineEdit(
        s.value(QStringLiteral("api/baseUrl"), AppConfig::defaultApiBaseUrl()).toString(), this);
    m_wsUrlEdit =
        new QLineEdit(s.value(QStringLiteral("api/wsUrl"), AppConfig::defaultWsBaseUrl()).toString(), this);
    m_apiUrlEdit->setPlaceholderText(QStringLiteral("例如 http://127.0.0.1:8080"));
    m_wsUrlEdit->setPlaceholderText(QStringLiteral("例如 ws://127.0.0.1:8080/ws"));
    pfLay->addRow(QStringLiteral("API 根地址"), m_apiUrlEdit);
    pfLay->addRow(QStringLiteral("WebSocket"), m_wsUrlEdit);

    auto *secBox = new QGroupBox(QStringLiteral("安全设置"), this);
    m_oldPwd = new QLineEdit(this);
    m_oldPwd->setEchoMode(QLineEdit::Password);
    m_oldPwd->setPlaceholderText(QStringLiteral("当前密码"));
    m_newPwd = new QLineEdit(this);
    m_newPwd->setEchoMode(QLineEdit::Password);
    m_newPwd->setPlaceholderText(QStringLiteral("新密码"));
    auto *chgBtn = new QPushButton(QStringLiteral("修改密码（演示请求）"), this);
    auto *secLay = new QFormLayout(secBox);
    secLay->addRow(QStringLiteral("原密码"), m_oldPwd);
    secLay->addRow(QStringLiteral("新密码"), m_newPwd);
    secLay->addRow(chgBtn);

    auto *notBox = new QGroupBox(QStringLiteral("通知设置"), this);
    m_notifyDesktop = new QCheckBox(QStringLiteral("桌面通知"), this);
    m_notifyDesktop->setChecked(s.value(QStringLiteral("notify/desktop"), true).toBool());
    m_notifySound = new QCheckBox(QStringLiteral("提示音"), this);
    m_notifySound->setChecked(s.value(QStringLiteral("notify/sound"), true).toBool());
    auto *nLay = new QVBoxLayout(notBox);
    nLay->addWidget(m_notifyDesktop);
    nLay->addWidget(m_notifySound);

    auto *avBox = new QGroupBox(QStringLiteral("头像"), this);
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(96, 96);
    m_avatarLabel->setFrameShape(QFrame::Box);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setText(QStringLiteral("暂无"));
    auto *avBtn = new QPushButton(QStringLiteral("上传头像…"), this);
    auto *avLay = new QHBoxLayout(avBox);
    avLay->addWidget(m_avatarLabel);
    avLay->addWidget(avBtn);
    avLay->addStretch();

    auto *saveBtn = new QPushButton(QStringLiteral("保存"), this);
    auto *cancelBtn = new QPushButton(QStringLiteral("关闭"), this);
    saveBtn->setDefault(true);
    auto *row = new QHBoxLayout();
    row->addStretch();
    row->addWidget(saveBtn);
    row->addWidget(cancelBtn);

    auto *main = new QVBoxLayout(this);
    main->addWidget(profileBox);
    main->addWidget(secBox);
    main->addWidget(notBox);
    main->addWidget(avBox);
    main->addLayout(row);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(avBtn, &QPushButton::clicked, this, &SettingsDialog::onPickAvatar);
    connect(chgBtn, &QPushButton::clicked, this, &SettingsDialog::onChangePwd);
}

void SettingsDialog::onPickAvatar()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择头像"), QString(),
                                                        QStringLiteral("图片 (*.png *.jpg *.jpeg)"));
    if (path.isEmpty())
        return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法读取文件"));
        return;
    }
    m_avatarData = f.readAll();
    m_avatarFileName = QFileInfo(path).fileName();
    QPixmap pm;
    if (pm.loadFromData(m_avatarData)) {
        m_avatarLabel->setPixmap(pm.scaled(m_avatarLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_avatarLabel->setText(QString());
    }
}

void SettingsDialog::onChangePwd()
{
    if (m_oldPwd->text().isEmpty() || m_newPwd->text().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请填写原密码与新密码"));
        return;
    }
    QJsonObject body;
    body.insert(QStringLiteral("oldPassword"), m_oldPwd->text());
    body.insert(QStringLiteral("newPassword"), m_newPwd->text());
    m_http->postJson(QStringLiteral("/api/doctor/change-password"), body,
                     [this](int status, const QJsonObject &resp, const QString &err) {
                         if (!err.isEmpty() && status <= 0) {
                             QMessageBox::critical(this, QStringLiteral("失败"), err);
                             return;
                         }
                         QMessageBox::information(this, QStringLiteral("结果"),
                                                  QStringLiteral("请求已发送，HTTP %1").arg(status));
                     });
}

void SettingsDialog::onSave()
{
    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    s.setValue(QStringLiteral("api/baseUrl"), m_apiUrlEdit->text().trimmed());
    s.setValue(QStringLiteral("api/wsUrl"), m_wsUrlEdit->text().trimmed());
    s.setValue(QStringLiteral("profile/name"), m_nameEdit->text());
    s.setValue(QStringLiteral("profile/title"), m_titleEdit->text());
    s.setValue(QStringLiteral("profile/phone"), m_phoneEdit->text());
    s.setValue(QStringLiteral("profile/dept"), m_deptEdit->text());
    s.setValue(QStringLiteral("notify/desktop"), m_notifyDesktop->isChecked());
    s.setValue(QStringLiteral("notify/sound"), m_notifySound->isChecked());

    if (!m_avatarData.isEmpty() && m_http) {
        m_http->uploadFile(QStringLiteral("/api/doctor/avatar"), QStringLiteral("file"), m_avatarData,
                           m_avatarFileName.isEmpty() ? QStringLiteral("avatar.png") : m_avatarFileName,
                           QJsonObject{},
                           [this](int status, const QJsonObject &resp, const QString &err) {
                               Q_UNUSED(resp);
                               if (!err.isEmpty())
                                   QMessageBox::warning(this, QStringLiteral("头像上传"), err);
                               else
                                   QMessageBox::information(this, QStringLiteral("头像上传"),
                                                            QStringLiteral("HTTP %1").arg(status));
                           });
    }
    QMessageBox::information(this, QStringLiteral("已保存"), QStringLiteral("设置已写入本地（头像走上传接口）"));
    accept();
}
