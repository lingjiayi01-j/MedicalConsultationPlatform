#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

class HttpClient;

/**
 * @brief 个人设置：资料、安全、通知、头像上传
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(HttpClient *http, QWidget *parent = nullptr);

private slots:
    void onSave();
    void onPickAvatar();
    void onChangePwd();

private:
    HttpClient *m_http = nullptr;

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_phoneEdit = nullptr;
    QLineEdit *m_deptEdit = nullptr;
    QLineEdit *m_apiUrlEdit = nullptr;
    QLineEdit *m_wsUrlEdit = nullptr;

    QLineEdit *m_oldPwd = nullptr;
    QLineEdit *m_newPwd = nullptr;

    QCheckBox *m_notifyDesktop = nullptr;
    QCheckBox *m_notifySound = nullptr;

    QLabel *m_avatarLabel = nullptr;
    QByteArray m_avatarData;
    QString m_avatarFileName;
};

#endif // SETTINGSDIALOG_H
