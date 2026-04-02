#include "prescriptiondialog.h"
#include "httpclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QAbstractItemView>
#include <QTableWidgetItem>

PrescriptionDialog::PrescriptionDialog(HttpClient *http, const QString &consultationId,
                                         const QJsonObject &patientInfo, QWidget *parent)
    : QDialog(parent), m_http(http), m_consultationId(consultationId), m_patient(patientInfo)
{
    setWindowTitle(QStringLiteral("开具电子处方"));
    resize(900, 520);

    const QString pname = m_patient.value(QStringLiteral("patientName")).toString();
    m_patientLabel = new QLabel(QStringLiteral("患者：%1").arg(pname), this);
    m_patientLabel->setStyleSheet(QStringLiteral("font-weight:600; font-size:14px;"));

    m_table = new QTableWidget(0, 7, this);
    const QStringList headers{QStringLiteral("药品名称"), QStringLiteral("剂型"), QStringLiteral("规格"),
                              QStringLiteral("用法用量"), QStringLiteral("天数"), QStringLiteral("备注"),
                              QStringLiteral("操作")};
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_globalRemark = new QPlainTextEdit(this);
    m_globalRemark->setPlaceholderText(QStringLiteral("处方总备注（选填）"));
    m_globalRemark->setMaximumHeight(80);

    m_signature = new QLineEdit(this);
    m_signature->setPlaceholderText(QStringLiteral("电子签名 / 签章"));

    auto *addBtn = new QPushButton(QStringLiteral("添加药品"), this);
    auto *submitBtn = new QPushButton(QStringLiteral("提交处方"), this);
    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    submitBtn->setDefault(true);

    auto *btnRow = new QHBoxLayout();
    btnRow->addWidget(addBtn);
    btnRow->addStretch();
    btnRow->addWidget(submitBtn);
    btnRow->addWidget(cancelBtn);

    auto *main = new QVBoxLayout(this);
    main->addWidget(m_patientLabel);
    main->addWidget(m_table);
    main->addWidget(new QLabel(QStringLiteral("总备注"), this));
    main->addWidget(m_globalRemark);
    main->addWidget(new QLabel(QStringLiteral("签名"), this));
    main->addWidget(m_signature);
    main->addLayout(btnRow);

    connect(addBtn, &QPushButton::clicked, this, &PrescriptionDialog::onAddDrug);
    connect(submitBtn, &QPushButton::clicked, this, &PrescriptionDialog::onSubmit);
    connect(cancelBtn, &QPushButton::clicked, this, &PrescriptionDialog::onCancel);

    onAddDrug();
}

void PrescriptionDialog::onAddDrug()
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    for (int c = 0; c < 6; ++c)
        m_table->setItem(row, c, new QTableWidgetItem());
    auto *delBtn = new QPushButton(QStringLiteral("删除"), this);
    m_table->setCellWidget(row, 6, delBtn);
    connect(delBtn, &QPushButton::clicked, this, [this, delBtn]() {
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->cellWidget(r, 6) == delBtn) {
                m_table->removeRow(r);
                break;
            }
        }
    });
}

void PrescriptionDialog::onCancel()
{
    reject();
}

bool PrescriptionDialog::validatePrescription() const
{
    if (m_signature->text().trimmed().isEmpty())
        return false;
    if (m_table->rowCount() == 0)
        return false;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        const QString name = m_table->item(r, 0) ? m_table->item(r, 0)->text().trimmed() : QString();
        if (name.isEmpty())
            return false;
    }
    return true;
}

QJsonObject PrescriptionDialog::buildPayload() const
{
    QJsonArray drugs;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QJsonObject d;
        auto cell = [this, r](int col) {
            return m_table->item(r, col) ? m_table->item(r, col)->text() : QString();
        };
        d.insert(QStringLiteral("name"), cell(0));
        d.insert(QStringLiteral("dosageForm"), cell(1));
        d.insert(QStringLiteral("spec"), cell(2));
        d.insert(QStringLiteral("usage"), cell(3));
        d.insert(QStringLiteral("days"), cell(4));
        d.insert(QStringLiteral("remark"), cell(5));
        drugs.append(d);
    }
    QJsonObject root;
    root.insert(QStringLiteral("consultationId"), m_consultationId);
    root.insert(QStringLiteral("patient"), m_patient);
    root.insert(QStringLiteral("drugs"), drugs);
    root.insert(QStringLiteral("globalRemark"), m_globalRemark->toPlainText());
    root.insert(QStringLiteral("signature"), m_signature->text().trimmed());
    return root;
}

void PrescriptionDialog::onSubmit()
{
    if (!validatePrescription()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"),
                             QStringLiteral("请填写签名，并确保每行药品名称完整。"));
        return;
    }
    const QString path =
        QStringLiteral("/api/consultations/%1/prescription").arg(m_consultationId);
    const QJsonObject payload = buildPayload();
    m_http->postJson(path, payload, [this](int status, const QJsonObject &resp, const QString &err) {
        if (!err.isEmpty() && status <= 0) {
            QMessageBox::critical(this, QStringLiteral("提交失败"), err);
            return;
        }
        const bool ok = resp.value(QStringLiteral("success")).toBool(true);
        if (status >= 200 && status < 300 && ok) {
            QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("处方已提交"));
            accept();
            return;
        }
        const QString msg = resp.value(QStringLiteral("message")).toString();
        QMessageBox::warning(this, QStringLiteral("提交失败"),
                             msg.isEmpty() ? QStringLiteral("服务器返回 %1").arg(status) : msg);
    });
}