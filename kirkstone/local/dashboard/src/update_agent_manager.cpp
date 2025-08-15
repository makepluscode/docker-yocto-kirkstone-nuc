#include "update_agent_manager.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>

// DLT context definition
DltContext UpdateAgentManager::m_ctx;

UpdateAgentManager::UpdateAgentManager(QObject *parent)
    : QObject(parent)
    , m_isServiceRunning(false)
    , m_isUpdateActive(false)
    , m_updateStatus("Idle")
    , m_updateProgress(0)
    , m_serviceStatusProcess(nullptr)
    , m_logFilePath("/var/log/update-agent.log")
    , m_lastLogPosition(0)
{
    // Initialize DLT
    DLT_REGISTER_CONTEXT(m_ctx, "UPAG", "Update Agent Manager");
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("UpdateAgentManager initialized"));

    // Setup refresh timer
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(5000); // Check every 5 seconds
    connect(m_refreshTimer, &QTimer::timeout, this, &UpdateAgentManager::onRefreshTimer);
    m_refreshTimer->start();

    // Setup log monitoring
    setupLogMonitoring();
    
    // Initial status check
    refresh();
}

UpdateAgentManager::~UpdateAgentManager()
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("UpdateAgentManager destroyed"));
    DLT_UNREGISTER_CONTEXT(m_ctx);
}

void UpdateAgentManager::refresh()
{
    checkServiceStatus();
    checkUpdateStatus();
}

void UpdateAgentManager::checkServiceStatus()
{
    if (m_serviceStatusProcess && m_serviceStatusProcess->state() != QProcess::NotRunning) {
        return; // Already checking
    }

    if (m_serviceStatusProcess) {
        m_serviceStatusProcess->deleteLater();
    }

    m_serviceStatusProcess = new QProcess(this);
    connect(m_serviceStatusProcess, QOverload<int>::of(&QProcess::finished),
            this, &UpdateAgentManager::onServiceStatusProcessFinished);

    // Check if update-agent service is running
    m_serviceStatusProcess->start("systemctl", QStringList() << "is-active" << "update-agent.service");
}

void UpdateAgentManager::onServiceStatusProcessFinished(int exitCode)
{
    bool wasRunning = m_isServiceRunning;
    m_isServiceRunning = (exitCode == 0);
    
    if (wasRunning != m_isServiceRunning) {
        DLT_LOG(m_ctx, DLT_LOG_INFO, 
                DLT_STRING("Service status changed:"), 
                DLT_STRING(m_isServiceRunning ? "running" : "stopped"));
        emit serviceStatusChanged();
        
        // If service stopped, clear update status
        if (!m_isServiceRunning && m_isUpdateActive) {
            m_isUpdateActive = false;
            m_updateStatus = "Service stopped";
            m_updateProgress = 0;
            emit updateStatusChanged();
            emit updateProgressChanged();
        }
    }

    m_serviceStatusProcess->deleteLater();
    m_serviceStatusProcess = nullptr;
}

void UpdateAgentManager::setupLogMonitoring()
{
    m_logWatcher = new QFileSystemWatcher(this);
    connect(m_logWatcher, &QFileSystemWatcher::fileChanged,
            this, &UpdateAgentManager::onLogFileChanged);

    // Watch systemd journal for update-agent service
    // Note: We'll monitor journalctl output instead of a file since update-agent uses DLT
    
    // Try to find the log file, fallback to monitoring systemd journal
    QStringList possibleLogPaths = {
        "/var/log/update-agent.log",
        "/tmp/update-agent.log",
        "/var/log/dlt.log"
    };
    
    for (const QString& path : possibleLogPaths) {
        QFile file(path);
        if (file.exists()) {
            m_logFilePath = path;
            m_logWatcher->addPath(path);
            DLT_LOG(m_ctx, DLT_LOG_INFO, 
                    DLT_STRING("Monitoring log file:"), 
                    DLT_STRING(path.toUtf8().constData()));
            break;
        }
    }
}

void UpdateAgentManager::onLogFileChanged(const QString& path)
{
    Q_UNUSED(path)
    parseLogContent();
}

void UpdateAgentManager::parseLogContent()
{
    QFile file(m_logFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    // Seek to last read position
    if (m_lastLogPosition > 0) {
        file.seek(m_lastLogPosition);
    }

    QTextStream stream(&file);
    QString line;
    
    while (stream.readLineInto(&line)) {
        parseLogLine(line);
    }
    
    m_lastLogPosition = file.pos();
    file.close();
}

void UpdateAgentManager::parseLogLine(const QString& line)
{
    // Parse different update states from log lines
    updateStatusFromLog(line);
    updateProgressFromLog(line);
}

void UpdateAgentManager::updateStatusFromLog(const QString& line)
{
    QString newStatus = m_updateStatus;
    bool wasUpdateActive = m_isUpdateActive;
    
    // Detect update start
    if (line.contains("Starting update process", Qt::CaseInsensitive) ||
        line.contains("Update available", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Update starting...";
        if (!wasUpdateActive) {
            emit updateStarted();
        }
    }
    // Detect download phase
    else if (line.contains("downloading", Qt::CaseInsensitive) ||
             line.contains("download", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Downloading update...";
    }
    // Detect installation phase
    else if (line.contains("installing", Qt::CaseInsensitive) ||
             line.contains("install", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Installing update...";
    }
    // Detect completion
    else if (line.contains("Update completed", Qt::CaseInsensitive) ||
             line.contains("Update successful", Qt::CaseInsensitive)) {
        m_isUpdateActive = false;
        newStatus = "Update completed successfully";
        m_updateProgress = 100;
        emit updateCompleted(true, "Update completed successfully");
    }
    // Detect failure
    else if (line.contains("Update failed", Qt::CaseInsensitive) ||
             line.contains("Failed to", Qt::CaseInsensitive)) {
        m_isUpdateActive = false;
        newStatus = "Update failed";
        emit updateCompleted(false, "Update failed");
    }
    
    if (newStatus != m_updateStatus) {
        m_updateStatus = newStatus;
        DLT_LOG(m_ctx, DLT_LOG_INFO, 
                DLT_STRING("Update status changed:"), 
                DLT_STRING(newStatus.toUtf8().constData()));
        emit updateStatusChanged();
    }
}

void UpdateAgentManager::updateProgressFromLog(const QString& line)
{
    // Look for progress indicators
    QRegularExpression progressRegex(R"(progress[:\s]*(\d+)%)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = progressRegex.match(line);
    
    if (match.hasMatch()) {
        int progress = match.captured(1).toInt();
        if (progress != m_updateProgress) {
            m_updateProgress = progress;
            emit updateProgressChanged();
        }
    }
    
    // Estimate progress from status
    if (m_isUpdateActive) {
        if (line.contains("downloading", Qt::CaseInsensitive)) {
            if (m_updateProgress < 30) m_updateProgress = 30;
        } else if (line.contains("installing", Qt::CaseInsensitive)) {
            if (m_updateProgress < 70) m_updateProgress = 70;
        }
        emit updateProgressChanged();
    }
}

void UpdateAgentManager::checkUpdateStatus()
{
    // Use journalctl to check recent update-agent logs
    QProcess* journalProcess = new QProcess(this);
    connect(journalProcess, QOverload<int>::of(&QProcess::finished),
            [this, journalProcess](int exitCode) {
        if (exitCode == 0) {
            QString output = journalProcess->readAllStandardOutput();
            QStringList lines = output.split('\n');
            
            // Process recent log lines
            for (const QString& line : lines) {
                if (!line.isEmpty()) {
                    parseLogLine(line);
                }
            }
        }
        journalProcess->deleteLater();
    });
    
    // Get last 50 lines from update-agent service
    journalProcess->start("journalctl", QStringList() 
                         << "-u" << "update-agent.service" 
                         << "--lines=50" 
                         << "--no-pager"
                         << "--since=10 minutes ago");
}

void UpdateAgentManager::onRefreshTimer()
{
    refresh();
}

void UpdateAgentManager::startService()
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Starting update-agent service"));
    
    QProcess* startProcess = new QProcess(this);
    connect(startProcess, QOverload<int>::of(&QProcess::finished),
            [this, startProcess](int exitCode) {
        DLT_LOG(m_ctx, DLT_LOG_INFO, 
                DLT_STRING("Start service result:"), 
                DLT_INT(exitCode));
        startProcess->deleteLater();
        // Refresh status after a short delay
        QTimer::singleShot(1000, this, &UpdateAgentManager::refresh);
    });
    
    startProcess->start("systemctl", QStringList() << "start" << "update-agent.service");
}

void UpdateAgentManager::stopService()
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Stopping update-agent service"));
    
    QProcess* stopProcess = new QProcess(this);
    connect(stopProcess, QOverload<int>::of(&QProcess::finished),
            [this, stopProcess](int exitCode) {
        DLT_LOG(m_ctx, DLT_LOG_INFO, 
                DLT_STRING("Stop service result:"), 
                DLT_INT(exitCode));
        stopProcess->deleteLater();
        // Refresh status after a short delay
        QTimer::singleShot(1000, this, &UpdateAgentManager::refresh);
    });
    
    stopProcess->start("systemctl", QStringList() << "stop" << "update-agent.service");
}