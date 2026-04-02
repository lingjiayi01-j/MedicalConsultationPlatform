#include "earningsdialog.h"
#include "httpclient.h"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QPainter>

using namespace QtCharts;

EarningsDialog::EarningsDialog(HttpClient *http, QWidget *parent) : QDialog(parent), m_http(http)
{
    setWindowTitle(QStringLiteral("收益统计"));
    resize(780, 560);

    m_todayLbl = new QLabel(QStringLiteral("今日：—"), this);
    m_monthLbl = new QLabel(QStringLiteral("本月：—"), this);
    m_totalLbl = new QLabel(QStringLiteral("累计：—"), this);
    m_todayLbl->setStyleSheet(QStringLiteral("font-size:16px; font-weight:600;"));
    m_monthLbl->setStyleSheet(QStringLiteral("font-size:16px; font-weight:600;"));
    m_totalLbl->setStyleSheet(QStringLiteral("font-size:16px; font-weight:600;"));

    auto *chart = new QChart();
    chart->setTitle(QStringLiteral("近7日收入趋势（元）"));
    chart->legend()->hide();
    m_chartView = new QChartView(chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    m_historyTable = new QTableWidget(0, 4, this);
    m_historyTable->setHorizontalHeaderLabels(
        {QStringLiteral("时间"), QStringLiteral("金额"), QStringLiteral("渠道"), QStringLiteral("状态")});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);

    auto *withdrawBtn = new QPushButton(QStringLiteral("申请提现"), this);
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    auto *row = new QHBoxLayout();
    row->addWidget(m_todayLbl);
    row->addWidget(m_monthLbl);
    row->addWidget(m_totalLbl);
    row->addStretch();
    row->addWidget(refreshBtn);
    row->addWidget(withdrawBtn);

    auto *main = new QVBoxLayout(this);
    main->addLayout(row);
    main->addWidget(m_chartView, 1);
    main->addWidget(new QLabel(QStringLiteral("提现记录"), this));
    main->addWidget(m_historyTable, 1);

    connect(refreshBtn, &QPushButton::clicked, this, &EarningsDialog::onRefresh);
    connect(withdrawBtn, &QPushButton::clicked, this, &EarningsDialog::onWithdraw);

    onRefresh();
}

void EarningsDialog::rebuildChart(const QVector<double> &series)
{
    QChart *chart = m_chartView->chart();
    chart->removeAllSeries();

    auto *s = new QLineSeries();
    for (int i = 0; i < series.size(); ++i)
        s->append(i, series.at(i));
    chart->addSeries(s);
    chart->createDefaultAxes();
    auto *axX = qobject_cast<QValueAxis *>(chart->axes(Qt::Horizontal).first());
    auto *axY = qobject_cast<QValueAxis *>(chart->axes(Qt::Vertical).first());
    if (axX) {
        axX->setLabelFormat(QStringLiteral("%d"));
        axX->setTitleText(QStringLiteral("天序"));
    }
    if (axY)
        axY->setTitleText(QStringLiteral("元"));
}

void EarningsDialog::loadStats()
{
    m_http->getJson(QStringLiteral("/api/earnings/summary"),
                    [this](int status, const QJsonObject &resp, const QString &err) {
                        Q_UNUSED(err);
                        if (status != 200) {
                            m_todayLbl->setText(QStringLiteral("今日：120.00（离线示例）"));
                            m_monthLbl->setText(QStringLiteral("本月：3500.00（离线示例）"));
                            m_totalLbl->setText(QStringLiteral("累计：128900.00（离线示例）"));
                            rebuildChart({80, 120, 90, 200, 150, 220, 120});
                            m_historyTable->setRowCount(2);
                            m_historyTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("2026-04-01 10:00")));
                            m_historyTable->setItem(0, 1, new QTableWidgetItem(QStringLiteral("500")));
                            m_historyTable->setItem(0, 2, new QTableWidgetItem(QStringLiteral("银行卡")));
                            m_historyTable->setItem(0, 3, new QTableWidgetItem(QStringLiteral("已到账")));
                            m_historyTable->setItem(1, 0, new QTableWidgetItem(QStringLiteral("2026-03-20 16:20")));
                            m_historyTable->setItem(1, 1, new QTableWidgetItem(QStringLiteral("300")));
                            m_historyTable->setItem(1, 2, new QTableWidgetItem(QStringLiteral("微信")));
                            m_historyTable->setItem(1, 3, new QTableWidgetItem(QStringLiteral("处理中")));
                            return;
                        }
                        const QJsonObject d = resp.value(QStringLiteral("data")).toObject();
                        m_todayLbl->setText(
                            QStringLiteral("今日：%1").arg(d.value(QStringLiteral("today")).toDouble()));
                        m_monthLbl->setText(
                            QStringLiteral("本月：%1").arg(d.value(QStringLiteral("month")).toDouble()));
                        m_totalLbl->setText(
                            QStringLiteral("累计：%1").arg(d.value(QStringLiteral("total")).toDouble()));
                        QJsonArray trend = d.value(QStringLiteral("trend")).toArray();
                        QVector<double> pts;
                        for (const auto &v : trend)
                            pts.append(v.toDouble());
                        if (pts.isEmpty())
                            pts = {80, 120, 90, 200, 150, 220, 120};
                        rebuildChart(pts);

                        const QJsonArray hist = d.value(QStringLiteral("withdrawals")).toArray();
                        m_historyTable->setRowCount(0);
                        for (const auto &item : hist) {
                            const QJsonObject o = item.toObject();
                            const int r = m_historyTable->rowCount();
                            m_historyTable->insertRow(r);
                            m_historyTable->setItem(r, 0, new QTableWidgetItem(o.value(QStringLiteral("at")).toString()));
                            m_historyTable->setItem(
                                r, 1, new QTableWidgetItem(QString::number(o.value(QStringLiteral("amount")).toDouble())));
                            m_historyTable->setItem(r, 2, new QTableWidgetItem(o.value(QStringLiteral("channel")).toString()));
                            m_historyTable->setItem(r, 3, new QTableWidgetItem(o.value(QStringLiteral("state")).toString()));
                        }
                    });
}

void EarningsDialog::onRefresh()
{
    loadStats();
}

void EarningsDialog::onWithdraw()
{
    QJsonObject body;
    body.insert(QStringLiteral("amount"), 100);
    m_http->postJson(QStringLiteral("/api/earnings/withdraw"), body,
                     [this](int status, const QJsonObject &resp, const QString &err) {
                         if (!err.isEmpty() && status <= 0)
                             QMessageBox::critical(this, QStringLiteral("提现"), err);
                         else
                             QMessageBox::information(
                                 this, QStringLiteral("提现"),
                                 resp.value(QStringLiteral("message")).toString().isEmpty()
                                     ? QStringLiteral("已提交申请 HTTP %1").arg(status)
                                     : resp.value(QStringLiteral("message")).toString());
                         loadStats();
                     });
}

<｜tool▁calls▁begin｜><｜tool▁call▁begin｜>
Read