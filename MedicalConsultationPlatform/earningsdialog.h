#ifndef EARNINGSDIALOG_H
#define EARNINGSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTableWidget>
#include <QtCharts/QChartView>

/**
 * @brief 收益统计：今日/本月/累计、趋势图、提现与记录
 */
class EarningsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EarningsDialog(HttpClient *http, QWidget *parent = nullptr);

private slots:
    void onWithdraw();
    void onRefresh();

private:
    void loadStats();
    void rebuildChart(const QVector<double> &series);

    HttpClient *m_http = nullptr;
    QLabel *m_todayLbl = nullptr;
    QLabel *m_monthLbl = nullptr;
    QLabel *m_totalLbl = nullptr;
    QTableWidget *m_historyTable = nullptr;
    QtCharts::QChartView *m_chartView = nullptr;
};

#endif // EARNINGSDIALOG_H
