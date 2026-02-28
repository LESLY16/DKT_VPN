#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QTime>
#include "vpnmanager.h"
#include "vpnserver.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onConnectClicked();
    void onStatusChanged(VpnStatus status, const QString &message);
    void onStatsUpdated(quint64 bytesRx, quint64 bytesTx);
    void onLogMessage(const QString &line);
    void updateConnectionTime();

private:
    void setupUi();
    void applyStyles();
    void updateStatusIndicator(VpnStatus status);
    static QString formatBytes(quint64 bytes);

    // UI widgets
    QWidget     *m_centralWidget  = nullptr;
    QLabel      *m_titleLabel     = nullptr;
    QLabel      *m_statusDot      = nullptr;
    QLabel      *m_statusLabel    = nullptr;
    QComboBox   *m_serverCombo    = nullptr;
    QPushButton *m_connectBtn     = nullptr;
    QLabel      *m_rxLabel        = nullptr;
    QLabel      *m_txLabel        = nullptr;
    QLabel      *m_timeLabel      = nullptr;
    QTextEdit   *m_logView        = nullptr;

    // Logic
    VpnManager           *m_vpnManager = nullptr;
    QList<VpnServer>      m_servers;
    QTimer               *m_connTimer  = nullptr;
    QTime                 m_connStart;
    VpnStatus             m_currentStatus = VpnStatus::Disconnected;
};
