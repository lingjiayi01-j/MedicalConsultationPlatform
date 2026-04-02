#ifndef PRESCRIPTIONDIALOG_H
#define PRESCRIPTIONDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>

class HttpClient;

/**
 * @brief 开具处方：药品表格 + 校验 + POST /api/consultations/{id}/prescription
 */
class PrescriptionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PrescriptionDialog(HttpClient *http, const QString &consultationId,
                                const QJsonObject &patientInfo, QWidget *parent = nullptr);

private slots:
    void onAddDrug();
    void onSubmit();
    void onCancel();

private:
    bool validatePrescription() const;
    QJsonObject buildPayload() const;

    HttpClient *m_http = nullptr;
    QString m_consultationId;
    QJsonObject m_patient;

    QLabel *m_patientLabel = nullptr;
    QTableWidget *m_table = nullptr;
    QPlainTextEdit *m_globalRemark = nullptr;
    QLineEdit *m_signature = nullptr;
};

#endif // PRESCRIPTIONDIALOG_H