#include "update_agent_manager.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>

// DLT context definition
DltContext UpdateAgentManager::m_ctx;

UpdateAgentManager::UpdateAgentManager(QObject *parent)
    : QObject(parent)
    , m_isServiceRunning(false)
    , m_isUpdateActive(false)  // Start as inactive
    , m_updateStatus("Polling")  // Start with Polling status
    , m_updateProgress(0)      // Start with 0% progress
    , m_serviceStatusProcess(nullptr)
    , m_currentOperation("idle")  // Start with idle operation
    , m_dbusConnection(QDBusConnection::systemBus())
    , m_dbusConnected(false)
{
    // Initialize DLT
    DLT_REGISTER_CONTEXT(m_ctx, "UPDM", "Update Agent Manager");
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("UpdateAgentManager initialized"));

    // Setup refresh timer
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000); // Check every 2 seconds
    connect(m_refreshTimer, &QTimer::timeout, this, &UpdateAgentManager::onRefreshTimer);
    m_refreshTimer->start();

    // Setup operation polling timer (disabled for now to prevent automatic status changes)
    m_operationPollTimer = new QTimer(this);
    m_operationPollTimer->setInterval(1000); // Poll operation every 1 second
    connect(m_operationPollTimer, &QTimer::timeout, this, &UpdateAgentManager::pollOperation);
    // m_operationPollTimer->start(); // Commented out to prevent automatic polling

    // Setup reboot progress timer
    m_rebootProgressTimer = new QTimer(this);
    m_rebootProgressTimer->setInterval(1000); // 1 second interval
    connect(m_rebootProgressTimer, &QTimer::timeout, this, [this]() {
        if (m_updateStatus == "Rebooting" && m_updateProgress < 99) {
            m_updateProgress++;
            emit updateProgressChanged();
            DLT_LOG(m_ctx, DLT_LOG_INFO,
                    DLT_STRING("Rebooting progress:"),
                    DLT_INT(m_updateProgress), DLT_STRING("%"));
        }
    });

    // Setup D-Bus monitoring
    setupDBusMonitoring();

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

void UpdateAgentManager::pollOperation()
{
    if (!m_dbusConnected) {
        return;
    }

    // Get Operation property from UpdateService
    QDBusInterface updateService("org.freedesktop.UpdateService",
                                "/org/freedesktop/UpdateService",
                                "org.freedesktop.UpdateService",
                                QDBusConnection::systemBus());

    if (!updateService.isValid()) {
        return;
    }

    QVariant property = updateService.property("Operation");
    if (property.isValid()) {
        QString operation = property.toString();
        if (operation != m_currentOperation) {
            m_currentOperation = operation;
            updateStatusFromOperation(operation);
            DLT_LOG(m_ctx, DLT_LOG_INFO,
                    DLT_STRING("Operation changed:"),
                    DLT_STRING(operation.toUtf8().constData()));
        }
    }

    // Also poll Progress property directly from UpdateService (safely)
    QVariant progressProperty = updateService.property("Progress");
    if (progressProperty.isValid() && !progressProperty.isNull()) {
        // Try to extract progress safely without QDBusArgument
        DLT_LOG(m_ctx, DLT_LOG_DEBUG,
                DLT_STRING("Progress property type:"),
                DLT_STRING(progressProperty.typeName()));
    }
}

void UpdateAgentManager::setupDBusMonitoring()
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Setting up D-Bus monitoring"));

    // Connect to system D-Bus
    if (!m_dbusConnection.isConnected()) {
        DLT_LOG(m_ctx, DLT_LOG_ERROR, DLT_STRING("Failed to connect to system D-Bus"));
        return;
    }

    // Connect to update service signals
    bool connected = m_dbusConnection.connect(
        "org.freedesktop.UpdateService",  // service
        "/org/freedesktop/UpdateService", // path
        "org.freedesktop.UpdateService",  // interface
        "Progress",                       // signal
        this,                            // receiver
        SLOT(onDBusSignal(QDBusMessage)) // slot
    );

    if (connected) {
        DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Connected to UpdateService Progress signal"));
    } else {
        DLT_LOG(m_ctx, DLT_LOG_ERROR, DLT_STRING("Failed to connect to UpdateService Progress signal"));
    }

    // Connect to Completed signal
    connected = m_dbusConnection.connect(
        "org.freedesktop.UpdateService",  // service
        "/org/freedesktop/UpdateService", // path
        "org.freedesktop.UpdateService",  // interface
        "Completed",                      // signal
        this,                            // receiver
        SLOT(onDBusSignal(QDBusMessage)) // slot
    );

    if (connected) {
        DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Connected to UpdateService Completed signal"));
    } else {
        DLT_LOG(m_ctx, DLT_LOG_ERROR, DLT_STRING("Failed to connect to UpdateService Completed signal"));
    }

    m_dbusConnected = true;
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("D-Bus monitoring setup completed"));
}

void UpdateAgentManager::updateStatusFromOperation(const QString& operation)
{
    QString newStatus = m_updateStatus;
    bool wasUpdateActive = m_isUpdateActive;

    if (operation == "idle") {
        m_isUpdateActive = false;
        newStatus = "Polling";
        m_updateProgress = 8; // Polling shows 8%
    } else if (operation == "installing") {
        m_isUpdateActive = true;
        newStatus = "Installing";
        if (!wasUpdateActive) {
            m_updateProgress = 30; // Installing starts at 30%
            emit updateStarted();
            emit updateProgressChanged();
        }
    } else if (operation.contains("download", Qt::CaseInsensitive) ||
               operation.contains("fetch", Qt::CaseInsensitive)) {
        m_isUpdateActive = true;
        newStatus = "Download";
        if (!wasUpdateActive) {
            m_updateProgress = 20; // Download shows 20%
            emit updateStarted();
            emit updateProgressChanged();
        }
    } else if (!operation.isEmpty() && operation != "idle") {
        // Any other non-idle operation indicates activity
        m_isUpdateActive = true;
        newStatus = "Download"; // Default to download phase for unknown operations
        if (!wasUpdateActive) {
            m_updateProgress = 20; // Default to download progress
            emit updateStarted();
            emit updateProgressChanged();
        }
    }

    if (newStatus != m_updateStatus || wasUpdateActive != m_isUpdateActive) {
        m_updateStatus = newStatus;
        emit updateStatusChanged();

        if (m_updateProgress == 0 && m_isUpdateActive && newStatus != "Installing") {
            emit updateProgressChanged();
        }
    }
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

    // Cycle through test states with new progress ranges
    static int testState = 0;
    testState = (testState + 1) % 5; // 5 states: Polling, Download, Installing, Installing, Rebooting

    bool oldActive = m_isUpdateActive;

    switch(testState) {
        case 0: // Polling
            m_isUpdateActive = false;
            m_updateStatus = "Polling";
            m_updateProgress = 8;
            break;
        case 1: // Download
            m_isUpdateActive = true;
            m_updateStatus = "Download";
            m_updateProgress = 20;
            if (!oldActive) emit updateStarted();
            break;
        case 2: // Installing (mid-range)
            m_isUpdateActive = true;
            m_updateStatus = "Installing";
            m_updateProgress = 50; // 30 + (40/2) = 50% (simulates 40% RAUC progress)
            break;
        case 3: // Installing (near end)
            m_isUpdateActive = true;
            m_updateStatus = "Installing";
            m_updateProgress = 75; // 30 + (90/2) = 75% (simulates 90% RAUC progress)
            break;
        case 4: // Rebooting (will auto-increment to 99%)
            m_isUpdateActive = true;
            m_updateStatus = "Rebooting";
            m_updateProgress = 80;
            m_rebootProgressTimer->start(); // Start reboot countdown
            break;
    }

    // Ensure signals are emitted
    emit updateStatusChanged();
    emit updateProgressChanged();

    DLT_LOG(m_ctx, DLT_LOG_INFO,
            DLT_STRING("Test results - State:"), DLT_INT(testState),
            DLT_STRING(", Progress:"), DLT_INT(m_updateProgress),
            DLT_STRING("%, Status:"), DLT_STRING(m_updateStatus.toUtf8().constData()),
            DLT_STRING(", Active:"), DLT_BOOL(m_isUpdateActive));
}

void UpdateAgentManager::onDBusSignal(const QDBusMessage& message)
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("=== D-BUS SIGNAL RECEIVED ==="));
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Interface:"), DLT_STRING(message.interface().toUtf8().constData()));
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Member:"), DLT_STRING(message.member().toUtf8().constData()));
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Sender:"), DLT_STRING(message.service().toUtf8().constData()));

    if (message.interface() == "org.freedesktop.UpdateService") {
        DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Processing UpdateService signal:"), DLT_STRING(message.member().toUtf8().constData()));

        if (message.member() == "Progress") {
            QList<QVariant> arguments = message.arguments();
            if (arguments.size() >= 1) {
                bool ok;
                int progress = arguments[0].toInt(&ok);
                if (ok) {
                    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Progress signal received:"), DLT_INT(progress), DLT_STRING("%"));
                    handleProgressSignal(progress);
                } else {
                    DLT_LOG(m_ctx, DLT_LOG_ERROR, DLT_STRING("Progress signal has wrong argument type"));
                }
            } else {
                DLT_LOG(m_ctx, DLT_LOG_ERROR, DLT_STRING("Progress signal missing arguments"));
            }
        } else if (message.member() == "Completed") {
            QList<QVariant> arguments = message.arguments();
            if (arguments.size() >= 2) {
                bool success = arguments[0].toBool();
                QString message_text = arguments[1].toString();
                DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Completed signal - Success:"), DLT_BOOL(success), DLT_STRING(", Message:"), DLT_STRING(message_text.toUtf8().constData()));
                handleCompletedSignal(success, message_text);
            } else {
                DLT_LOG(m_ctx, DLT_LOG_ERROR, DLT_STRING("Completed signal missing arguments"));
            }
        } else {
            DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Unknown UpdateService signal:"), DLT_STRING(message.member().toUtf8().constData()));
        }
    } else {
        DLT_LOG(m_ctx, DLT_LOG_DEBUG, DLT_STRING("Ignoring signal from interface:"), DLT_STRING(message.interface().toUtf8().constData()));
    }
}

void UpdateAgentManager::handleProgressSignal(int percentage)
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Handling RAUC progress signal:"), DLT_INT(percentage), DLT_STRING("%"));

    // Convert RAUC progress (0-100%) to Installing range (30-80%)
    // Formula: 30 + RAUC% / 2
    int mappedProgress = 30 + (percentage / 2);
    if (mappedProgress > 80) {
        mappedProgress = 80; // Cap at 80% for Installing phase
    }

    // Update progress with mapped value
    if (mappedProgress != m_updateProgress && percentage >= 0 && percentage <= 100) {
        m_updateProgress = mappedProgress;
        emit updateProgressChanged();

        DLT_LOG(m_ctx, DLT_LOG_INFO,
                DLT_STRING("RAUC progress:"), DLT_INT(percentage),
                DLT_STRING("% mapped to:"), DLT_INT(mappedProgress), DLT_STRING("%"));
    }

    // Update status based on progress
    if (percentage > 0 && !m_isUpdateActive) {
        m_isUpdateActive = true;
        emit updateStarted();
    }

    // Set status to Installing when receiving progress signals
    QString newStatus = "Installing";
    if (newStatus != m_updateStatus) {
        m_updateStatus = newStatus;
        emit updateStatusChanged();

        DLT_LOG(m_ctx, DLT_LOG_INFO,
                DLT_STRING("Status changed to:"),
                DLT_STRING(newStatus.toUtf8().constData()));
    }
}

void UpdateAgentManager::handleCompletedSignal(bool success, const QString& message)
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Handling completed signal - Success:"), DLT_BOOL(success), DLT_STRING(", Message:"), DLT_STRING(message.toUtf8().constData()));

    if (success) {
        // Start Rebooting phase at 80%
        m_isUpdateActive = true; // Keep active during reboot
        m_updateProgress = 80;
        m_updateStatus = "Rebooting";

        // Start reboot progress timer (80% -> 99%, 1% per second)
        m_rebootProgressTimer->start();

        DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Started Rebooting phase at 80%"));
    } else {
        m_isUpdateActive = false;
        m_updateProgress = 0;
        m_updateStatus = "Update failed: " + message;
    }

    emit updateCompleted(success, message);
    emit updateStatusChanged();
    emit updateProgressChanged();

    // After reboot sequence completes (or failure), reset to Polling state
    if (success) {
        QTimer::singleShot(22000, this, [this]() { // 22 seconds (80->99% takes ~19 seconds + 3 sec buffer)
            m_rebootProgressTimer->stop();
            m_isUpdateActive = false;
            m_updateStatus = "Polling";
            m_updateProgress = 8;
            emit updateStatusChanged();
            emit updateProgressChanged();
        });
    }
}



void UpdateAgentManager::testRealtimeMonitoring()
{
    DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Testing real-time monitoring - Operation polling active"));
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
