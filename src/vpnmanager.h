#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QString>
#include "vpnserver.h"

/// Current state of the VPN connection.
enum class VpnStatus {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Error
};

/**
 * VpnManager manages WireGuard VPN connections on all three platforms:
 *   - Linux / macOS : wg-quick up/down  (with pkexec / sudo for privileges)
 *   - Windows       : wireguard.exe /installtunnelservice and /uninstalltunnelservice
 *
 * It also polls `wg show` every 2 s to refresh transfer statistics while
 * a tunnel is active.
 */
class VpnManager : public QObject
{
    Q_OBJECT

public:
    explicit VpnManager(QObject *parent = nullptr);
    ~VpnManager() override;

    void connectToServer(const VpnServer &server);
    void disconnect();

    VpnStatus status() const { return m_status; }
    QString   currentServerName() const { return m_currentServerName; }

    /// Returns the directory where .conf files are read from.
    QString configDirectory() const;

signals:
    void statusChanged(VpnStatus status, const QString &message);
    void statsUpdated(quint64 bytesRx, quint64 bytesTx);
    void logMessage(const QString &line);

private slots:
    void onConnectFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDisconnectFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void pollStats();

private:
    // Helpers
    void   setStatus(VpnStatus s, const QString &msg = {});
    QString resolveConfigFile(const QString &configName) const;
    QString wgQuickPath() const;
    QString wireguardExePath() const;
    void   runConnectCommand(const QString &configFile);
    void   runDisconnectCommand();
    void   parseWgShowOutput(const QString &output);

    QProcess *m_connectProcess    = nullptr;
    QProcess *m_disconnectProcess = nullptr;
    QProcess *m_statsProcess      = nullptr;
    QTimer   *m_pollTimer         = nullptr;

    VpnStatus m_status            = VpnStatus::Disconnected;
    QString   m_currentServerName;
    QString   m_currentConfigName; ///< tunnel name used for disconnect
    QString   m_currentConfigFile; ///< full path to config file
};
