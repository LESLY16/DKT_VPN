#include "mainwindow.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QFrame>
#include <QSizePolicy>

// ── Constructor ───────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_servers    = defaultServers();
    m_vpnManager = new VpnManager(this);
    m_connTimer  = new QTimer(this);
    m_connTimer->setInterval(1000);

    setupUi();
    applyStyles();

    connect(m_vpnManager, &VpnManager::statusChanged,
            this, &MainWindow::onStatusChanged);
    connect(m_vpnManager, &VpnManager::statsUpdated,
            this, &MainWindow::onStatsUpdated);
    connect(m_vpnManager, &VpnManager::logMessage,
            this, &MainWindow::onLogMessage);
    connect(m_connTimer, &QTimer::timeout,
            this, &MainWindow::updateConnectionTime);
    connect(m_connectBtn, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
}

// ── UI setup ──────────────────────────────────────────────────────────────────
void MainWindow::setupUi()
{
    setWindowTitle("DKT VPN");
    setMinimumSize(480, 620);
    resize(480, 620);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto *rootLayout = new QVBoxLayout(m_centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────────
    auto *headerWidget = new QWidget;
    headerWidget->setObjectName("header");
    headerWidget->setFixedHeight(90);
    auto *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 12, 20, 12);

    m_titleLabel = new QLabel("DKT VPN");
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    auto *subtitleLabel = new QLabel("Secure WireGuard VPN");
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(subtitleLabel);
    rootLayout->addWidget(headerWidget);

    // ── Content ───────────────────────────────────────────────────────────────
    auto *contentWidget = new QWidget;
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(16);

    // Status row
    auto *statusRow = new QHBoxLayout;
    m_statusDot = new QLabel;
    m_statusDot->setObjectName("statusDot");
    m_statusDot->setFixedSize(14, 14);

    m_statusLabel = new QLabel("Disconnected");
    m_statusLabel->setObjectName("statusLabel");

    statusRow->addWidget(m_statusDot);
    statusRow->addSpacing(8);
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch();
    contentLayout->addLayout(statusRow);

    // Separator
    auto *sep1 = new QFrame;
    sep1->setFrameShape(QFrame::HLine);
    sep1->setObjectName("separator");
    contentLayout->addWidget(sep1);

    // Server selection group
    auto *serverGroup = new QGroupBox("Select Server");
    serverGroup->setObjectName("serverGroup");
    auto *serverLayout = new QVBoxLayout(serverGroup);
    serverLayout->setContentsMargins(12, 16, 12, 12);

    m_serverCombo = new QComboBox;
    m_serverCombo->setObjectName("serverCombo");
    m_serverCombo->setIconSize(QSize(24, 18));
    for (const VpnServer &srv : m_servers) {
        m_serverCombo->addItem(srv.flag + "  " + srv.country);
    }
    serverLayout->addWidget(m_serverCombo);
    contentLayout->addWidget(serverGroup);

    // Connect button
    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setObjectName("connectBtn");
    m_connectBtn->setFixedHeight(48);
    m_connectBtn->setCursor(Qt::PointingHandCursor);
    contentLayout->addWidget(m_connectBtn);

    // Separator
    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setObjectName("separator");
    contentLayout->addWidget(sep2);

    // Stats group
    auto *statsGroup = new QGroupBox("Connection Details");
    statsGroup->setObjectName("statsGroup");
    auto *statsGrid = new QGridLayout(statsGroup);
    statsGrid->setContentsMargins(12, 16, 12, 12);
    statsGrid->setVerticalSpacing(8);
    statsGrid->setHorizontalSpacing(12);

    auto addStat = [&](int row, const QString &label, QLabel *&valueLabel) {
        auto *lbl = new QLabel(label + ":");
        lbl->setObjectName("statKey");
        valueLabel = new QLabel("—");
        valueLabel->setObjectName("statValue");
        statsGrid->addWidget(lbl,        row, 0);
        statsGrid->addWidget(valueLabel, row, 1);
    };
    addStat(0, "Duration",   m_timeLabel);
    addStat(1, "Downloaded", m_rxLabel);
    addStat(2, "Uploaded",   m_txLabel);
    contentLayout->addWidget(statsGroup);

    // Log view
    auto *logGroup = new QGroupBox("Log");
    logGroup->setObjectName("logGroup");
    auto *logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(8, 8, 8, 8);

    m_logView = new QTextEdit;
    m_logView->setObjectName("logView");
    m_logView->setReadOnly(true);
    m_logView->setFixedHeight(120);
    logLayout->addWidget(m_logView);
    contentLayout->addWidget(logGroup);

    contentLayout->addStretch();
    rootLayout->addWidget(contentWidget, 1);

    updateStatusIndicator(VpnStatus::Disconnected);
}

void MainWindow::applyStyles()
{
    qApp->setStyle("Fusion");

    setStyleSheet(R"(
        QMainWindow {
            background-color: #1e2030;
        }

        #header {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #4f46e5, stop:1 #7c3aed);
        }

        #titleLabel {
            color: white;
            font-size: 22px;
            font-weight: bold;
            letter-spacing: 2px;
        }

        #subtitleLabel {
            color: rgba(255,255,255,0.75);
            font-size: 11px;
        }

        QWidget {
            background-color: #1e2030;
            color: #e2e8f0;
            font-family: "Segoe UI", "SF Pro Display", sans-serif;
            font-size: 13px;
        }

        QGroupBox {
            border: 1px solid #2d3154;
            border-radius: 8px;
            margin-top: 8px;
            padding-top: 6px;
            color: #a0aec0;
            font-size: 11px;
            font-weight: bold;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }

        #statusDot {
            border-radius: 7px;
        }

        #statusLabel {
            font-size: 14px;
            font-weight: bold;
            color: #e2e8f0;
        }

        #serverCombo {
            background-color: #252840;
            color: #e2e8f0;
            border: 1px solid #3d4166;
            border-radius: 6px;
            padding: 8px 12px;
            font-size: 13px;
            min-height: 36px;
        }

        #serverCombo::drop-down {
            border: none;
            width: 24px;
        }

        #serverCombo QAbstractItemView {
            background-color: #252840;
            color: #e2e8f0;
            selection-background-color: #4f46e5;
        }

        #connectBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #4f46e5, stop:1 #4338ca);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
        }

        #connectBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #6366f1, stop:1 #4f46e5);
        }

        #connectBtn:pressed {
            background: #3730a3;
        }

        #connectBtn[connecting="true"] {
            background: #374151;
        }

        #statKey {
            color: #718096;
            font-size: 12px;
        }

        #statValue {
            color: #e2e8f0;
            font-size: 12px;
            font-weight: bold;
        }

        #separator {
            color: #2d3154;
        }

        QFrame[frameShape="4"] {
            color: #2d3154;
        }

        #logView {
            background-color: #141520;
            color: #a0aec0;
            border: 1px solid #2d3154;
            border-radius: 4px;
            font-family: monospace;
            font-size: 11px;
        }
    )");
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void MainWindow::onConnectClicked()
{
    if (m_currentStatus == VpnStatus::Disconnected || m_currentStatus == VpnStatus::Error) {
        int idx = m_serverCombo->currentIndex();
        if (idx < 0 || idx >= m_servers.size())
            return;
        m_vpnManager->connectToServer(m_servers[idx]);
    } else if (m_currentStatus == VpnStatus::Connected) {
        m_vpnManager->disconnect();
    }
}

void MainWindow::onStatusChanged(VpnStatus status, const QString &message)
{
    m_currentStatus = status;
    updateStatusIndicator(status);

    switch (status) {
    case VpnStatus::Disconnected:
        m_statusLabel->setText("Disconnected");
        m_connectBtn->setText("Connect");
        m_connectBtn->setEnabled(true);
        m_connTimer->stop();
        m_timeLabel->setText("—");
        m_rxLabel->setText("—");
        m_txLabel->setText("—");
        m_serverCombo->setEnabled(true);
        break;

    case VpnStatus::Connecting:
        m_statusLabel->setText("Connecting…");
        m_connectBtn->setText("Connecting…");
        m_connectBtn->setEnabled(false);
        m_serverCombo->setEnabled(false);
        break;

    case VpnStatus::Connected:
        m_statusLabel->setText("Connected");
        m_connectBtn->setText("Disconnect");
        m_connectBtn->setEnabled(true);
        m_connStart = QTime::currentTime();
        m_connTimer->start();
        break;

    case VpnStatus::Disconnecting:
        m_statusLabel->setText("Disconnecting…");
        m_connectBtn->setText("Disconnecting…");
        m_connectBtn->setEnabled(false);
        m_connTimer->stop();
        break;

    case VpnStatus::Error:
        m_statusLabel->setText("Error");
        m_connectBtn->setText("Connect");
        m_connectBtn->setEnabled(true);
        m_serverCombo->setEnabled(true);
        m_connTimer->stop();
        if (!message.isEmpty()) {
            QMessageBox::warning(this, "VPN Error", message);
        }
        break;
    }
}

void MainWindow::onStatsUpdated(quint64 bytesRx, quint64 bytesTx)
{
    m_rxLabel->setText(formatBytes(bytesRx));
    m_txLabel->setText(formatBytes(bytesTx));
}

void MainWindow::onLogMessage(const QString &line)
{
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty())
        return;
    m_logView->append(trimmed);
    // Auto-scroll to bottom
    QScrollBar *sb = m_logView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::updateConnectionTime()
{
    if (m_currentStatus != VpnStatus::Connected)
        return;
    int elapsed = m_connStart.secsTo(QTime::currentTime());
    if (elapsed < 0) elapsed += 86400; // midnight wrap
    int h = elapsed / 3600;
    int m = (elapsed % 3600) / 60;
    int s = elapsed % 60;
    m_timeLabel->setText(QString::asprintf("%02d:%02d:%02d", h, m, s));
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void MainWindow::updateStatusIndicator(VpnStatus status)
{
    QString color;
    switch (status) {
    case VpnStatus::Connected:     color = "#22c55e"; break; // green
    case VpnStatus::Connecting:
    case VpnStatus::Disconnecting: color = "#f59e0b"; break; // amber
    case VpnStatus::Error:         color = "#ef4444"; break; // red
    default:                       color = "#6b7280"; break; // gray
    }
    m_statusDot->setStyleSheet(
        QString("background-color: %1; border-radius: 7px;").arg(color));
}

QString MainWindow::formatBytes(quint64 bytes)
{
    if (bytes < 1024)
        return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024)
        return QString::number(bytes / 1024.0, 'f', 1) + " KiB";
    if (bytes < 1024ull * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024), 'f', 2) + " MiB";
    return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 2) + " GiB";
}
