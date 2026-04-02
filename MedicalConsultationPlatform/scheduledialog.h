#ifndef SCHEDULEDIALOG_H
#define SCHEDULEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QCheckBox>
#include <QJsonObject>

/**
 * @brief 排班设置：周一至周日 × 上午/下午/晚上，启用 + 时间段
 */
class ScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScheduleDialog(QWidget *parent = nullptr);

    QJsonObject scheduleJson() const;

private slots:
    void onSave();
    void onCancel();

private:
    void loadDefaults();
    QTableWidget *m_table = nullptr;
};

#endif // SCHEDULEDIALOG_H
