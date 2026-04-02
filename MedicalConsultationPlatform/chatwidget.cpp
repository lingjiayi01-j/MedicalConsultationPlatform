#include "chatwidget.h"
#include "httpclient.h"
#include "websocketclient.h"
#include "localdatabase.h"
#include "prescriptiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
ChatWidget::ChatWidget(HttpClient *http, WebSocketClient *ws, LocalDatabase *cache, QWidget *parent)
    : QWidget(parent), m_http(http), m_ws(ws), m_cache(cache)
{
    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet(QStringLiteral("padding:8px; background:#e8f4fc; font-weight:600;"));

    m_view = new QTextBrowser(this);
    m_view->setOpenExternalLinks(true);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("输入回复，Enter 发送"));

    auto *sendBtn = new QPushButton(QStringLiteral("发送"), this);
    auto *attBtn = new QPushButton(QStringLiteral("附件"), this);
    auto *endBtn = new QPushButton(QStringLiteral("结束咨询"), this);
    auto *rxBtn = new QPushButton(QStringLiteral("开具处方"), this);

    auto *bottom = new QHBoxLayout();
    bottom->addWidget(m_input, 1);
    bottom->addWidget(sendBtn);
    bottom->addWidget(attBtn);
    bottom->addWidget(endBtn);
    bottom->addWidget(rxBtn);

    auto *main = new QVBoxLayout(this);
    main->addWidget(m_infoLabel);
    main->addWidget(m_view, 1);
    main->addLayout(bottom);

    connect(sendBtn, &QPushButton::clicked, this, &ChatWidget::onSend);
    connect(attBtn, &QPushButton::clicked, this, &ChatWidget::onAttach);
    connect(endBtn, &QPushButton::clicked, this, &ChatWidget::onEnd);
    connect(rxBtn, &QPushButton::clicked, this, &ChatWidget::onPrescription);
    connect(m_input, &QLineEdit::returnPressed, this, &ChatWidget::onSend);

    m_resendTimer.setInterval(5000);
    connect(&m_resendTimer, &QTimer::timeout, this, &ChatWidget::tryResendPending);
    m_resendTimer.start();
}

void ChatWidget::setConsultation(const QString &id, const QJsonObject &patientMeta)
{
    m_consultationId = id;
    m_patient = patientMeta;
    const QString name = m_patient.value(QStringLiteral("patientName")).toString();
    m_infoLabel->setText(QStringLiteral("患者：%1   会话：%2").arg(name, id));
    m_view->clear();
    fetchHistory();
}

void ChatWidget::markRead()
{
    // 仅占位：可与服务器已读接口联动
}

void ChatWidget::fetchHistory()
{
    if (m_consultationId.isEmpty())
        return;
    const QString path = QStringLiteral("/api/consultations/%1/messages").arg(m_consultationId);
    m_http->getJson(path, [this](int status, const QJsonObject &resp, const QString &err) {
        QJsonArray arr;
        if (status == 200) {
            const QJsonObject data = resp.value(QStringLiteral("data")).toObject();
            arr = data.value(QStringLiteral("items")).toArray();
            if (arr.isEmpty())
                arr = resp.value(QStringLiteral("items")).toArray();
        } else {
            if (m_cache)
                arr = m_cache->loadCachedMessages(m_consultationId);
            if (arr.isEmpty())
                qWarning() << QStringLiteral("加载历史失败，使用空消息：") << err;
        }

        m_view->clear();
        if (m_cache && !arr.isEmpty())
            m_cache->cacheMessages(m_consultationId, arr);

        for (const auto &v : arr) {
            const QJsonObject o = v.toObject();
            const QString sender = o.value(QStringLiteral("sender")).toString();
            const bool fromDoc = (sender == QLatin1String("doctor"));
            const QString content = o.value(QStringLiteral("content")).toString();
            const QString att = o.value(QStringLiteral("attachmentUrl")).toString();
            appendBubble(fromDoc, content, att);
        }
        scrollToBottom();
    });
}

QString ChatWidget::bubbleHtml(bool fromDoctor, const QString &text, const QString &att) const
{
    const QString align = fromDoctor ? QStringLiteral("right") : QStringLiteral("left");
    const QString bg = fromDoctor ? QStringLiteral("#d1e7dd") : QStringLiteral("#ffffff");
    const QString border = fromDoctor ? QStringLiteral("#badbcc") : QStringLiteral("#cfe2ff");
    QString body = text.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
    if (!att.isEmpty())
        body += QStringLiteral("<br/><a href=\"%1\">查看附件</a>").arg(att.toHtmlEscaped());
    return QStringLiteral("<table width='100%'><tr><td align='%1'>"
                          "<div style='display:inline-block;max-width:70%%;text-align:left;"
                          "background:%2;border:1px solid %3;border-radius:10px;padding:8px;'>%4</div>"
                          "</td></tr></table>")
        .arg(align, bg, border, body);
}

void ChatWidget::appendBubble(bool fromDoctor, const QString &text, const QString &attachmentUrl)
{
    m_view->append(bubbleHtml(fromDoctor, text, attachmentUrl));
    if (!fromDoctor && !isVisible())
        emit flashRequested();
    scrollToBottom();
}

void ChatWidget::scrollToBottom()
{
    QTimer::singleShot(0, this, [this]() {
        QScrollBar *sb = m_view->verticalScrollBar();
        if (sb)
            sb->setValue(sb->maximum());
    });
}

void ChatWidget::onSend()
{
    const QString text = m_input->text().trimmed();
    if (text.isEmpty() || m_consultationId.isEmpty())
        return;
    m_input->clear();

    const QString cid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    appendBubble(true, text, QString());

    if (m_ws && m_ws->isConnected()) {
        m_ws->sendChatMessage(m_consultationId, text, cid);
    } else {
        PendingMsg p;
        p.clientId = cid;
        p.text = text;
        m_pending.insert(cid, p);
    }
}

void ChatWidget::handleIncomingJson(const QJsonObject &obj)
{
    const QString type = obj.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("chat") || type == QLatin1String("message")) {
        const QString cid = obj.value(QStringLiteral("consultationId")).toString();
        if (cid != m_consultationId)
            return;
        const QString sender = obj.value(QStringLiteral("sender")).toString();
        const bool fromDoc = (sender == QLatin1String("doctor"));
        const QString content = obj.value(QStringLiteral("content")).toString();
        const QString att = obj.value(QStringLiteral("attachmentUrl")).toString();
        appendBubble(fromDoc, content, att);
        const QString ackId = obj.value(QStringLiteral("clientMsgId")).toString();
        if (m_pending.contains(ackId))
            m_pending.remove(ackId);
    }
}

void ChatWidget::tryResendPending()
{
    if (!m_ws || !m_ws->isConnected())
        return;
    for (auto it = m_pending.begin(); it != m_pending.end();) {
        PendingMsg &p = it.value();
        if (p.retries >= 5) {
            it = m_pending.erase(it);
            continue;
        }
        m_ws->sendChatMessage(m_consultationId, p.text, p.clientId);
        p.retries++;
        ++it;
    }
}

void ChatWidget::onAttach()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择附件"));
    if (path.isEmpty())
        return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法读取文件"));
        return;
    }
    const QByteArray data = f.readAll();
    const QString fname = QFileInfo(path).fileName();
    QJsonObject extra;
    extra.insert(QStringLiteral("consultationId"), m_consultationId);
    m_http->uploadFile(QStringLiteral("/api/consultations/upload"), QStringLiteral("file"), data, fname, extra,
                       [this](int status, const QJsonObject &resp, const QString &err) {
                           if (status < 200 || status >= 300) {
                               QMessageBox::warning(this, QStringLiteral("上传失败"),
                                                    err.isEmpty() ? QStringLiteral("HTTP 错误") : err);
                               return;
                           }
                           const QString url = resp.value(QStringLiteral("url")).toString();
                           appendBubble(true, QStringLiteral("[附件]"), url);
                           if (m_ws && m_ws->isConnected())
                               m_ws->sendChatMessage(m_consultationId, QStringLiteral("[附件] ") + url, QString());
                       });
}

void ChatWidget::onEnd()
{
    if (m_consultationId.isEmpty())
        return;
    QJsonObject body;
    body.insert(QStringLiteral("consultationId"), m_consultationId);
    m_http->postJson(QStringLiteral("/api/consultations/close"), body,
                     [this](int status, const QJsonObject &, const QString &) {
                         QMessageBox::information(this, QStringLiteral("结束咨询"),
                                                  QStringLiteral("状态码：%1").arg(status));
                         emit consultationClosed(m_consultationId);
                     });
}

void ChatWidget::onPrescription()
{
    PrescriptionDialog dlg(m_http, m_consultationId, m_patient, this);
    dlg.exec();
}
