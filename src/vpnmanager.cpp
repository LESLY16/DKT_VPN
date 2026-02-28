#include "vpnmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

// ── Platform guards ──────────────────────────────────────────────────────────
#ifdef Q_OS_WIN
#  include <windows.h>
#endif

// ────────────────────────────────────────────────────────────────────────────
VpnManager::VpnManager(QObject *parent)
    : QObject(parent)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(2000);
    connect(m_pollTimer, &QTimer::timeout, this, &VpnManager::pollStats);
}

VpnManager::~VpnManager()
{
    if (m_connectProcess) {
        m_connectProcess->kill();
        m_connectProcess->waitForFinished(1000);
    }
    if (m_disconnectProcess) {
        m_disconnectProcess->kill();
        m_disconnectProcess->waitForFinished(1000);
    }
    if (m_statsProcess) {
        m_statsProcess->kill();
        m_statsProcess->waitForFinished(1000);
    }
}

// ── Public API ───────────────────────────────────────────────────────────────
void VpnManager::connectToServer(const VpnServer &server)
{
    if (m_status == VpnStatus::Connecting || m_status == VpnStatus::Connected)
        return;

    QString configFile = resolveConfigFile(server.configName);
    if (configFile.isEmpty()) {
        setStatus(VpnStatus::Error,
                  tr("Config file not found for %1.\n"
                     "Copy configs/%2.conf.template to configs/%2.conf "
                     "and fill in your credentials.")
                  .arg(server.country, server.configName));
        return;
    }

    m_currentServerName = server.country;
    m_currentConfigName = server.configName;
    m_currentConfigFile = configFile;

    setStatus(VpnStatus::Connecting, tr("Connecting to %1…").arg(server.country));
    runConnectCommand(configFile);
}

void VpnManager::disconnect()
{
    if (m_status == VpnStatus::Disconnected || m_status == VpnStatus::Disconnecting)
        return;

    m_pollTimer->stop();
    setStatus(VpnStatus::Disconnecting, tr("Disconnecting…"));
    runDisconnectCommand();
}

// ── Config directory resolution ──────────────────────────────────────────────
QString VpnManager::configDirectory() const
{
    // 1. Environment variable override
    QString envDir = qEnvironmentVariable("DKT_VPN_CONFIG_DIR");
    if (!envDir.isEmpty() && QDir(envDir).exists())
        return envDir;

    // 2. User config location
    QString userCfg = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!userCfg.isEmpty()) {
        QDir d(userCfg);
        if (d.exists() || d.mkpath("."))
            return userCfg;
    }

    // 3. configs/ next to the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString bundled = appDir + "/configs";
    if (QDir(bundled).exists())
        return bundled;

    return appDir;
}

QString VpnManager::resolveConfigFile(const QString &configName) const
{
    QStringList searchDirs;

    QString envDir = qEnvironmentVariable("DKT_VPN_CONFIG_DIR");
    if (!envDir.isEmpty())
        searchDirs << envDir;

    searchDirs << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QString appDir = QCoreApplication::applicationDirPath();
    searchDirs << appDir + "/configs"
               << appDir;

    for (const QString &dir : searchDirs) {
        QString path = dir + "/" + configName + ".conf";
        if (QFileInfo::exists(path))
            return path;
    }
    return {};
}

// ── Platform-specific command helpers ────────────────────────────────────────
QString VpnManager::wgQuickPath() const
{
#ifdef Q_OS_WIN
    return {};
#else
    for (const char *p : { "/usr/bin/wg-quick",
                           "/usr/local/bin/wg-quick",
                           "/opt/homebrew/bin/wg-quick" }) {
        if (QFileInfo::exists(QString::fromUtf8(p)))
            return QString::fromUtf8(p);
    }
    return "wg-quick"; // rely on PATH
#endif
}

QString VpnManager::wireguardExePath() const
{
#ifdef Q_OS_WIN
    for (const char *p : { "C:\\Program Files\\WireGuard\\wireguard.exe",
                           "C:\\Program Files (x86)\\WireGuard\\wireguard.exe" }) {
        if (QFileInfo::exists(QString::fromUtf8(p)))
            return QString::fromUtf8(p);
    }
    return "wireguard.exe";
#else
    return {};
#endif
}

void VpnManager::runConnectCommand(const QString &configFile)
{
    if (m_connectProcess) {
        m_connectProcess->kill();
        m_connectProcess->deleteLater();
    }
    m_connectProcess = new QProcess(this);
    m_connectProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_connectProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        emit logMessage(QString::fromLocal8Bit(m_connectProcess->readAllStandardOutput()));
    });
    connect(m_connectProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VpnManager::onConnectFinished);
    connect(m_connectProcess, &QProcess::errorOccurred,
            this, &VpnManager::onProcessError);

#ifdef Q_OS_WIN
    // Windows: install the WireGuard tunnel service (requires Administrator)
    QString wgExe = wireguardExePath();
    m_connectProcess->start(wgExe, { "/installtunnelservice", configFile });
#elif defined(Q_OS_LINUX)
    // Linux: try pkexec for graphical privilege escalation, fall back to sudo
    QString wgQuick = wgQuickPath();
    if (QFileInfo::exists("/usr/bin/pkexec")) {
        m_connectProcess->start("/usr/bin/pkexec", { wgQuick, "up", configFile });
    } else {
        m_connectProcess->start("sudo", { wgQuick, "up", configFile });
    }
#else
    // macOS
    QString wgQuick = wgQuickPath();
    m_connectProcess->start("sudo", { wgQuick, "up", configFile });
#endif
}

void VpnManager::runDisconnectCommand()
{
    if (m_disconnectProcess) {
        m_disconnectProcess->kill();
        m_disconnectProcess->deleteLater();
    }
    m_disconnectProcess = new QProcess(this);
    m_disconnectProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_disconnectProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        emit logMessage(QString::fromLocal8Bit(m_disconnectProcess->readAllStandardOutput()));
    });
    connect(m_disconnectProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VpnManager::onDisconnectFinished);
    connect(m_disconnectProcess, &QProcess::errorOccurred,
            this, &VpnManager::onProcessError);

#ifdef Q_OS_WIN
    QString wgExe = wireguardExePath();
    m_disconnectProcess->start(wgExe, { "/uninstalltunnelservice", m_currentConfigName });
#elif defined(Q_OS_LINUX)
    QString wgQuick = wgQuickPath();
    if (QFileInfo::exists("/usr/bin/pkexec")) {
        m_disconnectProcess->start("/usr/bin/pkexec", { wgQuick, "down", m_currentConfigFile });
    } else {
        m_disconnectProcess->start("sudo", { wgQuick, "down", m_currentConfigFile });
    }
#else
    QString wgQuick = wgQuickPath();
    m_disconnectProcess->start("sudo", { wgQuick, "down", m_currentConfigFile });
#endif
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void VpnManager::onConnectFinished(int exitCode, QProcess::ExitStatus)
{
    if (exitCode == 0) {
        setStatus(VpnStatus::Connected,
                  tr("Connected to %1").arg(m_currentServerName));
        m_pollTimer->start();
    } else {
        QString out;
        if (m_connectProcess)
            out = QString::fromLocal8Bit(m_connectProcess->readAllStandardOutput());
        setStatus(VpnStatus::Error,
                  tr("Failed to connect (exit code %1).\n%2").arg(exitCode).arg(out));
    }
}

void VpnManager::onDisconnectFinished(int exitCode, QProcess::ExitStatus)
{
    if (exitCode == 0) {
        m_currentServerName.clear();
        m_currentConfigName.clear();
        m_currentConfigFile.clear();
        setStatus(VpnStatus::Disconnected, tr("Disconnected"));
    } else {
        // Even on error, treat as disconnected to allow retry
        setStatus(VpnStatus::Error,
                  tr("Disconnect may have failed (exit code %1). "
                     "Check tunnel status manually.").arg(exitCode));
    }
}

void VpnManager::onProcessError(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
    case QProcess::FailedToStart:
        msg = tr("WireGuard command not found. "
                 "Please install WireGuard and ensure it is in your PATH.");
        break;
    case QProcess::Crashed:
        msg = tr("WireGuard process crashed unexpectedly.");
        break;
    default:
        msg = tr("Process error: %1").arg(error);
        break;
    }
    setStatus(VpnStatus::Error, msg);
}

void VpnManager::pollStats()
{
    if (m_status != VpnStatus::Connected)
        return;

    if (m_statsProcess && m_statsProcess->state() != QProcess::NotRunning)
        return; // previous poll still running

    if (!m_statsProcess) {
        m_statsProcess = new QProcess(this);
        m_statsProcess->setProcessChannelMode(QProcess::MergedChannels);
    }

    // Capture output when finished
    connect(m_statsProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
                if (exitCode == 0) {
                    QString out = QString::fromLocal8Bit(m_statsProcess->readAllStandardOutput());
                    parseWgShowOutput(out);
                }
                m_statsProcess->disconnect();
            });

#ifdef Q_OS_WIN
    m_statsProcess->start(wireguardExePath(), { "/show", m_currentConfigName });
#else
    // wg show <tunnel>
    QString wgBin;
    for (const char *p : { "/usr/bin/wg", "/usr/local/bin/wg", "/opt/homebrew/bin/wg" }) {
        if (QFileInfo::exists(QString::fromUtf8(p))) { wgBin = QString::fromUtf8(p); break; }
    }
    if (wgBin.isEmpty()) wgBin = "wg";
    m_statsProcess->start(wgBin, { "show", m_currentConfigName });
#endif
}

void VpnManager::parseWgShowOutput(const QString &output)
{
    quint64 rx = 0, tx = 0;
    static const QRegularExpression rxRe(R"(transfer:\s*([\d\.]+)\s*(\w+)\s+received,\s*([\d\.]+)\s*(\w+)\s+sent)");
    QRegularExpressionMatch m = rxRe.match(output);
    if (m.hasMatch()) {
        auto toBytes = [](const QString &val, const QString &unit) -> quint64 {
            double v = val.toDouble();
            QString u = unit.toLower();
            if (u == "kib") v *= 1024.0;
            else if (u == "mib") v *= 1024.0 * 1024.0;
            else if (u == "gib") v *= 1024.0 * 1024.0 * 1024.0;
            return static_cast<quint64>(v);
        };
        rx = toBytes(m.captured(1), m.captured(2));
        tx = toBytes(m.captured(3), m.captured(4));
    }
    emit statsUpdated(rx, tx);
}

// ── Internal helpers ──────────────────────────────────────────────────────────
void VpnManager::setStatus(VpnStatus s, const QString &msg)
{
    m_status = s;
    emit statusChanged(s, msg);
    if (!msg.isEmpty())
        emit logMessage(msg);
}
