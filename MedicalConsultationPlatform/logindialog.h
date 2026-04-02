#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class HttpClient;
class QKeyEvent;

/**
 * @brief 医生端登录：工号/手机 + 密码，记住密码（加密），POST /api/auth/login
 */
class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(HttpClient *http, QWidget *parent = nullptr);

    QString authToken() const { return m_token; }
    QString doctorName() const { return m_doctorName; }
    QString doctorId() const { return m_doctorId; }

signals:
    void loginSucceeded(const QString &token, const QString &doctorName, const QString &doctorId);

private slots:
    void onLoginClicked();
    void tryLoginFromEnter();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void loadRemembered();
    void saveRemembered();
    void clearError();

    HttpClient *m_http = nullptr;
    QLineEdit *m_accountEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QCheckBox *m_rememberCheck = nullptr;
    QPushButton *m_loginBtn = nullptr;
    QLabel *m_errorLabel = nullptr;

    QString m_token;
    QString m_doctorName;
    QString m_doctorId;
};

#endif // LOGINDIALOG_H
