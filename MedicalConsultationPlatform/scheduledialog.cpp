#include "scheduledialog.h"
#include "appconfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTimeEdit>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTableWidgetItem>
#include <QWidget>
#include <QTime>

ScheduleDialog::ScheduleDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("排班设置"));
    resize(920, 360);

    m_table = new QTableWidget(3, 8, this);
    const QStringList rows{QStringLiteral("上午"), QStringLiteral("下午"), QStringLiteral("晚上")};
    const QStringList cols{QStringLiteral("时段"), QStringLiteral("周一"), QStringLiteral("周二"),
                           QStringLiteral("周三"), QStringLiteral("周四"), QStringLiteral("周五"),
                           QStringLiteral("周六"), QStringLiteral("周日")};
    m_table->setHorizontalHeaderLabels(cols);
    m_table->setVerticalHeaderLabels(rows);

    for (int r = 0; r < 3; ++r) {
        for (int c = 1; c < 8; ++c) {
            auto *cell = new QWidget(this);
            auto *lay = new QVBoxLayout(cell);
            lay->setContentsMargins(4, 4, 4, 4);
            auto *enable = new QCheckBox(QStringLiteral("启用"), cell);
            auto *start = new QTimeEdit(cell);
            auto *end = new QTimeEdit(cell);
            start->setDisplayFormat(QStringLiteral("HH:mm"));
            end->setDisplayFormat(QStringLiteral("HH:mm"));
            if (r == 0) {
                start->setTime(QTime(8, 0));
                end->setTime(QTime(12, 0));
            } else if (r == 1) {
                start->setTime(QTime(14, 0));
                end->setTime(QTime(18, 0));
            } else {
                start->setTime(QTime(18, 0));
                end->setTime(QTime(21, 0));
            }
            lay->addWidget(enable);
            lay->addWidget(start);
            lay->addWidget(end);
            m_table->setCellWidget(r, c, cell);
        }
    }

    auto *saveBtn = new QPushButton(QStringLiteral("保存"), this);
    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    saveBtn->setDefault(true);
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);

    auto *main = new QVBoxLayout(this);
    main->addWidget(m_table);
    main->addLayout(btnRow);

    connect(saveBtn, &QPushButton::clicked, this, &ScheduleDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &ScheduleDialog::onCancel);

    loadDefaults();
}

void ScheduleDialog::loadDefaults()
{
    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    const QByteArray raw = s.value(QStringLiteral("schedule/json")).toByteArray();
    if (raw.isEmpty())
        return;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return;
    const QJsonArray arr = doc.array();
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 1; c < 8 && idx < arr.size(); ++c, ++idx) {
            const QJsonObject o = arr.at(idx).toObject();
            QWidget *w = qobject_cast<QWidget *>(m_table->cellWidget(r, c));
            if (!w)
                continue;
            auto boxes = w->findChildren<QCheckBox *>();
            auto times = w->findChildren<QTimeEdit *>();
            if (!boxes.isEmpty())
                boxes.first()->setChecked(o.value(QStringLiteral("enabled")).toBool());
            if (times.size() >= 2) {
                const QTime ts = QTime::fromString(o.value(QStringLiteral("start")).toString(), QStringLiteral("HH:mm"));
                const QTime te = QTime::fromString(o.value(QStringLiteral("end")).toString(), QStringLiteral("HH:mm"));
                if (ts.isValid())
                    times.at(0)->setTime(ts);
                if (te.isValid())
                    times.at(1)->setTime(te);
            }
        }
    }
}

QJsonObject ScheduleDialog::scheduleJson() const
{
    QJsonArray arr;
    const QStringList dayKeys{QStringLiteral("mon"), QStringLiteral("tue"), QStringLiteral("wed"),
                              QStringLiteral("thu"), QStringLiteral("fri"), QStringLiteral("sat"),
                              QStringLiteral("sun")};
    const QStringList slotKeys{QStringLiteral("morning"), QStringLiteral("afternoon"), QStringLiteral("evening")};
    for (int r = 0; r < 3; ++r) {
        for (int c = 1; c < 8; ++c) {
            QWidget *w = qobject_cast<QWidget *>(m_table->cellWidget(r, c));
            QJsonObject o;
            o.insert(QStringLiteral("day"), dayKeys.at(c - 1));
            o.insert(QStringLiteral("slot"), slotKeys.at(r));
            if (w) {
                auto boxes = w->findChildren<QCheckBox *>();
                auto times = w->findChildren<QTimeEdit *>();
                o.insert(QStringLiteral("enabled"), boxes.isEmpty() ? false : boxes.first()->isChecked());
                if (times.size() >= 2) {
                    o.insert(QStringLiteral("start"), times.at(0)->time().toString(QStringLiteral("HH:mm")));
                    o.insert(QStringLiteral("end"), times.at(1)->time().toString(QStringLiteral("HH:mm")));
                }
            }
            arr.append(o);
        }
    }
    QJsonObject root;
    root.insert(QStringLiteral("slots"), arr);
    return root;
}

void ScheduleDialog::onSave()
{
    QSettings s(AppConfig::organizationName(), AppConfig::applicationName());
    const QJsonObject root = scheduleJson();
    s.setValue(QStringLiteral("schedule/json"),
               QJsonDocument(root.value(QStringLiteral("slots")).toArray()).toJson(QJsonDocument::Compact));
    QMessageBox::information(this, QStringLiteral("已保存"), QStringLiteral("排班已保存到本地设置（可再接 API 同步）"));
    accept();
}

void ScheduleDialog::onCancel()
{
    reject();
}
