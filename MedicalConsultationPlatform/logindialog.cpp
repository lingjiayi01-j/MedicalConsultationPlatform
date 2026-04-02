#include "logindialog.h"
#include "httpclient.h"
#include "appconfig.h"
#include "cryptohelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSettings>
#include <QKeyEvent>
#include <QJsonObject>

LoginDialog::LoginDialog(HttpClient *http, QWidget *parent)
    : QDialog(parent), m_http(http)
{
    setWindowTitle(QStringLiteral("医疗咨询平台 — 医生登录"));
    setMinimumWidth(400);

    m_accountEdit = new QLineEdit(this);
    m_accountEdit->setPlaceholderText(QStringLiteral("工号 / 手机号"));
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_rememberCheck = new QCheckBox(QStringLiteral("记住密码"), this);
    m_loginBtn = new QPushButton(QStringLiteral("登录"), this);
    m_loginBtn->setDefault(true);
    m_loginBtn->setMinimumHeight(40);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #c62828;"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    auto *form = new QFormLayout();
    form->addRow(QStringLiteral("账号"), m_accountEdit);
    form->addRow(QStringLiteral("密码"), m_passwordEdit);

    auto *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addWidget(m_rememberCheck);
    main->addWidget(m_errorLabel);
    main->addWidget(m_loginBtn);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_accountEdit, &QLineEdit::textChanged, this, &LoginDialog::clearError);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &LoginDialog::clearError);

    loadRemembered();
}

void LoginDialog::clearError()
{
    m_errorLabel->clear();
    m_errorLabel->hide();
}

void LoginDialog::loadRemembered()
{
    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    if (s.value(QStringLiteral("login/remember"), false).toBool()) {
        m_rememberCheck->setChecked(true);
        m_accountEdit->setText(s.value(QStringLiteral("login/account")).toString());
        const QString enc = s.value(QStringLiteral("login/passwordEnc")).toString();
        m_passwordEdit->setText(CryptoHelper::decryptPassword(enc));
    }
}

void LoginDialog::saveRemembered()
{
    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    if (m_rememberCheck->isChecked()) {
        s.setValue(QStringLiteral("login/remember"), true);
        s.setValue(QStringLiteral("login/account"), m_accountEdit->text().trimmed());
        s.setValue(QStringLiteral("login/passwordEnc"),
                   CryptoHelper::encryptPassword(m_passwordEdit->text()));
    } else {
        s.remove(QStringLiteral("login/remember"));
        s.remove(QStringLiteral("login/account"));
        s.remove(QStringLiteral("login/passwordEnc"));
    }
}

void LoginDialog::tryLoginFromEnter()
{
    if (focusWidget() == m_accountEdit && !m_passwordEdit->text().isEmpty()) {
        m_passwordEdit->setFocus();
        return;
    }
    if (focusWidget() == m_passwordEdit || focusWidget() == m_accountEdit)
        onLoginClicked();
}

void LoginDialog::onLoginClicked()
{
    clearError();
    const QString account = m_accountEdit->text().trimmed();
    const QString password = m_passwordEdit->text();
    if (account.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText(QStringLiteral("请输入账号和密码"));
        m_errorLabel->show();
        return;
    }

    m_loginBtn->setEnabled(false);
    QJsonObject body;
    body.insert(QStringLiteral("account"), account);
    body.insert(QStringLiteral("password"), password);

    m_http->postJson(QStringLiteral("/api/auth/login"), body,
                     [this](int status, const QJsonObject &resp, const QString &err) {
                         m_loginBtn->setEnabled(true);
                         if (!err.isEmpty() && status <= 0) {
                             m_errorLabel->setText(QStringLiteral("网络异常：%1").arg(err));
                             m_errorLabel->show();
                             return;
                         }
                         const bool ok = resp.value(QStringLiteral("success")).toBool(true);
                         if (status == 200 && ok) {
                             m_token = resp.value(QStringLiteral("token")).toString();
                             if (m_token.isEmpty())
                                 m_token = resp.value(QStringLiteral("data"))
                                               .toObject()
                                               .value(QStringLiteral("token"))
                                               .toString();
                             const QJsonObject data = resp.value(QStringLiteral("data")).toObject();
                             m_doctorName = data.value(QStringLiteral("doctorName")).toString();
                             m_doctorId = data.value(QStringLiteral("doctorId")).toString();
                             if (m_doctorName.isEmpty())
                                 m_doctorName = resp.value(QStringLiteral("doctorName")).toString();
                             if (m_doctorId.isEmpty())
                                 m_doctorId = resp.value(QStringLiteral("doctorId")).toString();

                             saveRemembered();
                             emit loginSucceeded(m_token, m_doctorName, m_doctorId);
                             accept();
                             return;
                         }
                         const QString msg = resp.value(QStringLiteral("message")).toString();
                         m_errorLabel->setText(msg.isEmpty()
                                                   ? QStringLiteral("登录失败（%1）").arg(status)
                                                   : msg);
                         m_errorLabel->show();
                     });
}

void LoginDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        tryLoginFromEnter();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}
