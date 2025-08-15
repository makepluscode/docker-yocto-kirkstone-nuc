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
    // Log the line for debugging
    if (!line.trimmed().isEmpty()) {
        DLT_LOG(m_ctx, DLT_LOG_DEBUG, 
                DLT_STRING("Parsing log line:"), 
                DLT_STRING(line.trimmed().toUtf8().constData()));
    }
    
    // Parse different update states from log lines
    updateStatusFromLog(line);
    updateProgressFromLog(line);
}

void UpdateAgentManager::updateStatusFromLog(const QString& line)
{
    QString newStatus = m_updateStatus;
    bool wasUpdateActive = m_isUpdateActive;
    
    // Detect update start
    if (line.contains("=== Starting update process ===", Qt::CaseInsensitive) ||
        line.contains("Starting update process", Qt::CaseInsensitive) ||
        line.contains("Update available", Qt::CaseInsensitive) ||
        line.contains("Deployment found", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Update starting...";
        if (!wasUpdateActive) {
            emit updateStarted();
        }
    }
    // Detect download phase
    else if (line.contains("downloading", Qt::CaseInsensitive) ||
             line.contains("download", Qt::CaseInsensitive) ||
             line.contains("Downloading bundle", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Downloading update...";
    }
    // Detect installation phase
    else if (line.contains("installing", Qt::CaseInsensitive) ||
             line.contains("install", Qt::CaseInsensitive) ||
             line.contains("Installing bundle", Qt::CaseInsensitive) ||
             line.contains("rauc install", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Installing update...";
    }
    // Detect verification phase
    else if (line.contains("verifying", Qt::CaseInsensitive) ||
             line.contains("verify", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Verifying update...";
    }
    // Detect completion
    else if (line.contains("Update completed: success", Qt::CaseInsensitive) ||
             line.contains("Update completed successfully", Qt::CaseInsensitive) ||
             line.contains("Installation completed", Qt::CaseInsensitive)) {
        m_isUpdateActive = false;
        newStatus = "Update completed successfully";
        m_updateProgress = 100;
        emit updateCompleted(true, "Update completed successfully");
    }
    // Detect failure
    else if (line.contains("Update completed: failure", Qt::CaseInsensitive) ||
             line.contains("Update failed", Qt::CaseInsensitive) ||
             line.contains("Failed to", Qt::CaseInsensitive) ||
             line.contains("Installation failed", Qt::CaseInsensitive)) {
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
    // Look for progress indicators in multiple formats
    QRegularExpression progressRegex1(R"(Update progress:\s*(\d+)%)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression progressRegex2(R"(progress[:\s]*(\d+)%)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression progressRegex3(R"((\d+)%)", QRegularExpression::CaseInsensitiveOption);
    
    QRegularExpressionMatch match = progressRegex1.match(line);
    if (!match.hasMatch()) {
        match = progressRegex2.match(line);
    }
    if (!match.hasMatch() && line.contains("progress", Qt::CaseInsensitive)) {
        match = progressRegex3.match(line);
    }
    
    if (match.hasMatch()) {
        int progress = match.captured(1).toInt();
        if (progress != m_updateProgress && progress >= 0 && progress <= 100) {
            m_updateProgress = progress;
            DLT_LOG(m_ctx, DLT_LOG_INFO, 
                    DLT_STRING("Progress updated:"), 
                    DLT_INT(progress));
            emit updateProgressChanged();
            return; // Exit early if we found explicit progress
        }
    }
    
    // Estimate progress from status keywords if no explicit progress found
    if (m_isUpdateActive) {
        bool progressUpdated = false;
        if (line.contains("Starting update process", Qt::CaseInsensitive)) {
            if (m_updateProgress < 5) {
                m_updateProgress = 5;
                progressUpdated = true;
            }
        } else if (line.contains("downloading", Qt::CaseInsensitive) || line.contains("download", Qt::CaseInsensitive)) {
            if (m_updateProgress < 30) {
                m_updateProgress = 30;
                progressUpdated = true;
            }
        } else if (line.contains("installing", Qt::CaseInsensitive) || line.contains("install", Qt::CaseInsensitive)) {
            if (m_updateProgress < 70) {
                m_updateProgress = 70;
                progressUpdated = true;
            }
        } else if (line.contains("verifying", Qt::CaseInsensitive) || line.contains("verify", Qt::CaseInsensitive)) {
            if (m_updateProgress < 90) {
                m_updateProgress = 90;
                progressUpdated = true;
            }
        }
        
        if (progressUpdated) {
            DLT_LOG(m_ctx, DLT_LOG_INFO, 
                    DLT_STRING("Progress estimated:"), 
                    DLT_INT(m_updateProgress));
            emit updateProgressChanged();
        }
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

void UpdateAgentManager::testProgressParsing(const QString& testLine)
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, 
            DLT_STRING("Testing progress parsing with line:"), 
            DLT_STRING(testLine.toUtf8().constData()));
    
    int oldProgress = m_updateProgress;
    QString oldStatus = m_updateStatus;
    
    parseLogLine(testLine);
    
    DLT_LOG(m_ctx, DLT_LOG_INFO, 
            DLT_STRING("Progress changed from"), DLT_INT(oldProgress),
            DLT_STRING("to"), DLT_INT(m_updateProgress));
    DLT_LOG(m_ctx, DLT_LOG_INFO, 
            DLT_STRING("Status changed from"), DLT_STRING(oldStatus.toUtf8().constData()),
            DLT_STRING("to"), DLT_STRING(m_updateStatus.toUtf8().constData()));
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