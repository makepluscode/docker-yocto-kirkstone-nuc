#include "system_info.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <QProcess>
#include <QStorageInfo>
#include <QHostInfo>
#include <QSysInfo>
#include <QRegExp>
#include <QCoreApplication>

// DLT context definition
DltContext SystemInfo::m_dltCtx;

// Helper macros for DLT logging
#define DLT_LOG_SYS_INFO(str) \
    do { \
        DLT_LOG(SystemInfo::m_dltCtx, \
                DLT_LOG_INFO, \
                DLT_STRING(str)); \
    } while(0)

#define DLT_LOG_SYS_WARN(str) \
    do { \
        DLT_LOG(SystemInfo::m_dltCtx, \
                DLT_LOG_WARN, \
                DLT_STRING(str)); \
    } while(0)

#define DLT_LOG_SYS_ERROR(str) \
    do { \
        DLT_LOG(SystemInfo::m_dltCtx, \
                DLT_LOG_ERROR, \
                DLT_STRING(str)); \
    } while(0)

SystemInfo::SystemInfo(QObject *parent)
    : QObject(parent)
    , m_cpuUsage(0.0)
    , m_memoryUsage(0.0)
    , m_totalMemory(0)
    , m_usedMemory(0)
    , m_freeMemory(0)
    , m_temperature(0.0)
    , m_networkConnected(false)
    , m_rootPartitionTotal(0)
    , m_rootPartitionUsed(0)
    , m_rootPartitionFree(0)
    , m_rootPartitionUsagePercent(0.0)
    , m_buildTime("")
    , m_yoctoVersion("")
    , m_rootDevice("")
    , m_lastCpuTotal(0)
    , m_lastCpuIdle(0)
{
    // Register DLT context (once per process)
    static bool dltContextRegistered = false;
    if (!dltContextRegistered) {
        DLT_REGISTER_CONTEXT(SystemInfo::m_dltCtx, "SYSM", "System Info Manager");
        dltContextRegistered = true;
        DLT_LOG_SYS_INFO("SystemInfo DLT context registered");
    }
    
    DLT_LOG_SYS_INFO("SystemInfo manager initialized");
    
    // Initialize system details that don't change frequently
    updateSystemDetails();
    updateBuildInfo();
    updateRootDeviceInfo();
    updateSoftwareVersion();

    // Setup timers
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &SystemInfo::updateSystemInfo);
    m_updateTimer->start(2000); // Update every 2 seconds

    m_timeTimer = new QTimer(this);
    connect(m_timeTimer, &QTimer::timeout, this, &SystemInfo::updateTime);
    m_timeTimer->start(1000); // Update time every second

    // Initial update
    updateSystemInfo();
    updateTime();
}

void SystemInfo::updateSystemInfo()
{
    updateCpuUsage();
    updateCpuCoreUsage();
    updateMemoryInfo();
    updateTemperature();
    updateUptime();
    updateNetworkInfo();
    updateDiskInfo();
}

void SystemInfo::updateTime()
{
    QString newTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    if (m_currentTime != newTime) {
        m_currentTime = newTime;
        emit currentTimeChanged();
    }
}

void SystemInfo::updateCpuUsage()
{
    QString statContent = readFileContent("/proc/stat");
    if (statContent.isEmpty()) return;

    QStringList lines = statContent.split('\n');
    if (lines.isEmpty()) return;

    QStringList cpuData = lines[0].split(' ', Qt::SkipEmptyParts);
    if (cpuData.size() < 8) return;

    qint64 idle = cpuData[4].toLongLong();
    qint64 total = 0;
    for (int i = 1; i < cpuData.size(); ++i) {
        total += cpuData[i].toLongLong();
    }

    if (m_lastCpuTotal != 0) {
        qint64 totalDiff = total - m_lastCpuTotal;
        qint64 idleDiff = idle - m_lastCpuIdle;

        if (totalDiff > 0) {
            double newCpuUsage = 100.0 * (totalDiff - idleDiff) / totalDiff;
            if (qAbs(m_cpuUsage - newCpuUsage) > 0.1) {
                m_cpuUsage = newCpuUsage;
                emit cpuUsageChanged();
            }
        }
    }

    m_lastCpuTotal = total;
    m_lastCpuIdle = idle;
}

void SystemInfo::updateCpuCoreUsage()
{
    QString statContent = readFileContent("/proc/stat");
    if (statContent.isEmpty()) return;

    QStringList lines = statContent.split('\n');
    QStringList newCpuCoreUsage;

    // Find all CPU core lines (cpu0, cpu1, cpu2, etc.)
    for (const QString &line : lines) {
        if (line.startsWith("cpu") && line != lines[0]) { // Skip the first "cpu" line (total)
            QStringList cpuData = line.split(' ', Qt::SkipEmptyParts);
            if (cpuData.size() < 8) continue;

            qint64 idle = cpuData[4].toLongLong();
            qint64 total = 0;
            for (int i = 1; i < cpuData.size(); ++i) {
                total += cpuData[i].toLongLong();
            }

            // Calculate usage percentage
            double usage = 0.0;
            if (total > 0) {
                usage = 100.0 * (total - idle) / total;
            }

            newCpuCoreUsage.append(QString::number(usage, 'f', 1));
        }
    }

    if (m_cpuCoreUsage != newCpuCoreUsage) {
        m_cpuCoreUsage = newCpuCoreUsage;
        emit cpuCoreUsageChanged();
    }
}

void SystemInfo::updateMemoryInfo()
{
    QString meminfoContent = readFileContent("/proc/meminfo");
    if (meminfoContent.isEmpty()) return;

    QStringList lines = meminfoContent.split('\n');
    qint64 memTotal = 0, memFree = 0, memAvailable = 0, buffers = 0, cached = 0;

    for (const QString &line : lines) {
        QStringList parts = line.split(':', Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        QString key = parts[0].trimmed();
        QString valueStr = parts[1].trimmed().split(' ')[0];
        qint64 value = valueStr.toLongLong() * 1024; // Convert from KB to bytes

        if (key == "MemTotal") memTotal = value;
        else if (key == "MemFree") memFree = value;
        else if (key == "MemAvailable") memAvailable = value;
        else if (key == "Buffers") buffers = value;
        else if (key == "Cached") cached = value;
    }

    qint64 newTotalMemory = memTotal;
    qint64 newUsedMemory = memTotal - memAvailable;
    qint64 newFreeMemory = memAvailable;
    double newMemoryUsage = memTotal > 0 ? (100.0 * newUsedMemory / memTotal) : 0.0;

    if (m_totalMemory != newTotalMemory) {
        m_totalMemory = newTotalMemory;
        emit totalMemoryChanged();
    }
    if (m_usedMemory != newUsedMemory) {
        m_usedMemory = newUsedMemory;
        emit usedMemoryChanged();
    }
    if (m_freeMemory != newFreeMemory) {
        m_freeMemory = newFreeMemory;
        emit freeMemoryChanged();
    }
    if (qAbs(m_memoryUsage - newMemoryUsage) > 0.1) {
        m_memoryUsage = newMemoryUsage;
        emit memoryUsageChanged();
    }
}

void SystemInfo::updateTemperature()
{
    // Try to read from coretemp (Intel CPU) first
    QString tempContent = readFileContent("/sys/class/hwmon/hwmon1/temp1_input");
    if (!tempContent.isEmpty()) {
        bool ok;
        double temp = tempContent.trimmed().toDouble(&ok);
        if (ok && temp > 1000) { // Ensure it's a reasonable temperature value
            double newTemperature = temp / 1000.0; // Convert from millidegrees to degrees
            if (qAbs(m_temperature - newTemperature) > 0.1) {
                m_temperature = newTemperature;
                emit temperatureChanged();
            }
            return;
        }
    }
    // Fallback: try thermal_zone0
    tempContent = readFileContent("/sys/class/thermal/thermal_zone0/temp");
    if (!tempContent.isEmpty()) {
        bool ok;
        double temp = tempContent.trimmed().toDouble(&ok);
        if (ok) {
            double newTemperature = temp / 1000.0;
            if (qAbs(m_temperature - newTemperature) > 0.1) {
                m_temperature = newTemperature;
                emit temperatureChanged();
            }
            return;
        }
    }
    // Fallback: try hwmon scan (as before)
    QProcess process;
    process.start("find", QStringList() << "/sys/class/hwmon" << "-name" << "temp*_input");
    process.waitForFinished(1000);
    QStringList tempFiles = QString::fromUtf8(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &tempFile : tempFiles) {
        QString content = readFileContent(tempFile);
        if (!content.isEmpty()) {
            bool ok;
            double temp = content.trimmed().toDouble(&ok);
            if (ok && temp > 1000) {
                double newTemperature = temp / 1000.0;
                if (qAbs(m_temperature - newTemperature) > 0.1) {
                    m_temperature = newTemperature;
                    emit temperatureChanged();
                }
                return;
            }
        }
    }
}

void SystemInfo::updateUptime()
{
    QString uptimeContent = readFileContent("/proc/uptime");
    if (uptimeContent.isEmpty()) return;

    QStringList parts = uptimeContent.split(' ');
    if (parts.isEmpty()) return;

    bool ok;
    double uptimeSeconds = parts[0].toDouble(&ok);
    if (!ok) return;

    int days = uptimeSeconds / 86400;
    int hours = (int(uptimeSeconds) % 86400) / 3600;
    int minutes = (int(uptimeSeconds) % 3600) / 60;
    int seconds = int(uptimeSeconds) % 60;

    QString newUptime = QString("%1d %2h %3m %4s").arg(days).arg(hours).arg(minutes).arg(seconds);
    if (m_uptime != newUptime) {
        m_uptime = newUptime;
        emit uptimeChanged();
    }
}

void SystemInfo::updateSystemDetails()
{
    QString newKernelVersion = QSysInfo::kernelVersion();
    QString newHostname = QHostInfo::localHostName();
    QString newArchitecture = QSysInfo::currentCpuArchitecture();

    if (m_kernelVersion != newKernelVersion) {
        m_kernelVersion = newKernelVersion;
        emit kernelVersionChanged();
    }
    if (m_hostname != newHostname) {
        m_hostname = newHostname;
        emit hostnameChanged();
    }
    if (m_architecture != newArchitecture) {
        m_architecture = newArchitecture;
        emit architectureChanged();
    }
}

void SystemInfo::updateNetworkInfo()
{
    bool connected = false;
    QString interface;
    QString ipAddr;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {

            QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (const QNetworkAddressEntry &entry : entries) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    connected = true;
                    interface = iface.name();
                    ipAddr = entry.ip().toString();
                    break;
                }
            }
            if (connected) break;
        }
    }

    if (m_networkConnected != connected) {
        m_networkConnected = connected;
        emit networkConnectedChanged();
    }
    if (m_networkInterface != interface) {
        m_networkInterface = interface;
        emit networkInterfaceChanged();
    }
    if (m_ipAddress != ipAddr) {
        m_ipAddress = ipAddr;
        emit ipAddressChanged();
    }
}

void SystemInfo::updateDiskInfo()
{
    QStorageInfo storage("/");

    qint64 newTotal = storage.bytesTotal();
    qint64 newFree = storage.bytesAvailable();
    qint64 newUsed = newTotal - newFree;
    double newUsagePercent = newTotal > 0 ? (100.0 * newUsed / newTotal) : 0.0;

    if (m_rootPartitionTotal != newTotal) {
        m_rootPartitionTotal = newTotal;
        emit rootPartitionTotalChanged();
    }
    if (m_rootPartitionUsed != newUsed) {
        m_rootPartitionUsed = newUsed;
        emit rootPartitionUsedChanged();
    }
    if (m_rootPartitionFree != newFree) {
        m_rootPartitionFree = newFree;
        emit rootPartitionFreeChanged();
    }
    if (qAbs(m_rootPartitionUsagePercent - newUsagePercent) > 0.1) {
        m_rootPartitionUsagePercent = newUsagePercent;
        emit rootPartitionUsagePercentChanged();
    }
}

QString SystemInfo::readFileContent(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);
    return stream.readAll();
}

void SystemInfo::updateRootDeviceInfo()
{
    // Read root device from /proc/mounts
    QString mountsContent = readFileContent("/proc/mounts");
    QString newRootDevice = "Unknown";

    if (!mountsContent.isEmpty()) {
        QStringList lines = mountsContent.split('\n');
        for (const QString &line : lines) {
            QStringList parts = line.split(' ');
            if (parts.size() >= 2 && parts[1] == "/") {
                QString device = parts[0];
                // Keep the full device path including partition (e.g., /dev/sda2)
                if (device.startsWith("/dev/")) {
                    newRootDevice = device;
                } else {
                    newRootDevice = device;
                }
                break;
            }
        }
    }

    if (m_rootDevice != newRootDevice) {
        m_rootDevice = newRootDevice;
        emit rootDeviceChanged();
    }
}

void SystemInfo::updateBuildInfo()
{
    // Read build time from /etc/buildinfo or similar
    QString buildTimeFile = "/etc/buildinfo";
    QString newBuildTime = readFileContent(buildTimeFile);
    if (newBuildTime.isEmpty()) {
        // Fallback: try to get from /proc/version
        QString procVersion = readFileContent("/proc/version");
        if (!procVersion.isEmpty()) {
            // Extract build date from kernel version string
            QRegExp rx("\\w+\\s+\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+\\s+\\w+\\s+\\d+");
            if (rx.indexIn(procVersion) != -1) {
                newBuildTime = rx.cap(0);
            }
        }
    }

    if (m_buildTime != newBuildTime) {
        m_buildTime = newBuildTime;
        emit buildTimeChanged();
    }

    // Read Yocto version from /etc/os-release
    QString osReleaseFile = "/etc/os-release";
    QString osReleaseContent = readFileContent(osReleaseFile);
    QString newYoctoVersion = "";

    if (!osReleaseContent.isEmpty()) {
        QStringList lines = osReleaseContent.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith("VERSION=")) {
                newYoctoVersion = line.mid(8).remove('"');
                break;
            }
        }
    }

    if (m_yoctoVersion != newYoctoVersion) {
        m_yoctoVersion = newYoctoVersion;
        emit yoctoVersionChanged();
    }
}

QString SystemInfo::formatBytes(qint64 bytes)
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}

void SystemInfo::refresh() {
    updateSystemInfo();
}

void SystemInfo::exitApplication() {
    DLT_LOG_SYS_INFO("Dashboard application exit requested");
    qDebug() << "Exiting application...";
    QCoreApplication::quit();
}

void SystemInfo::rebootSystem() {
    DLT_LOG_SYS_INFO("System reboot requested");
    qDebug() << "Rebooting system...";
    QProcess::startDetached("reboot", QStringList());
}

void SystemInfo::startHawkbitUpdater() {
    DLT_LOG_SYS_INFO("F1 Button pressed - Starting Hawkbit updater service...");
    qDebug() << "Starting Hawkbit updater service...";

    // First check if service is already running and stop it to ensure clean start
    DLT_LOG_SYS_INFO("Stopping existing Hawkbit service for clean start");
    QProcess stopProcess;
    stopProcess.start("systemctl", QStringList() << "stop" << "rauc-hawkbit-cpp.service");
    stopProcess.waitForFinished(5000);

    // Start the rauc-hawkbit-cpp service
    DLT_LOG_SYS_INFO("Starting rauc-hawkbit-cpp.service");
    QProcess startProcess;
    startProcess.start("systemctl", QStringList() << "start" << "rauc-hawkbit-cpp.service");
    startProcess.waitForFinished(5000);

    if (startProcess.exitCode() == 0) {
        DLT_LOG_SYS_INFO("Hawkbit updater service started successfully");
        qDebug() << "Hawkbit updater service started successfully";

        // Create the start signal file to trigger the hawkbit client to begin polling
        DLT_LOG_SYS_INFO("Creating start signal file: /tmp/rauc-hawkbit-start-signal");
        qDebug() << "Creating start signal file for rauc-hawkbit-cpp";
        QProcess signalProcess;
        signalProcess.start("touch", QStringList() << "/tmp/rauc-hawkbit-start-signal");
        signalProcess.waitForFinished(2000);

        if (signalProcess.exitCode() == 0) {
            DLT_LOG_SYS_INFO("Start signal file created successfully - Hawkbit will begin polling");
            qDebug() << "Start signal file created successfully";
            emit hawkbitServiceStatusChanged(true);
        } else {
            QString error = signalProcess.readAllStandardError();
            DLT_LOG_SYS_ERROR(QString("Failed to create start signal file: %1").arg(error).toUtf8().constData());
            qDebug() << "Failed to create start signal file:" << error;
            emit hawkbitUpdateFailed("Failed to create start signal file");
        }
    } else {
        QString error = startProcess.readAllStandardError();
        DLT_LOG_SYS_ERROR(QString("Failed to start Hawkbit service: %1").arg(error).toUtf8().constData());
        qDebug() << "Failed to start Hawkbit updater service:" << error;
        emit hawkbitUpdateFailed("Failed to start Hawkbit service");
    }
}

bool SystemInfo::checkHawkbitServiceStatus() {
    QProcess process;
    process.start("systemctl", QStringList() << "is-active" << "rauc-hawkbit-cpp.service");
    process.waitForFinished(3000);
    
    QString status = process.readAllStandardOutput().trimmed();
    bool isActive = (status == "active");
    
    DLT_LOG_SYS_INFO(QString("Hawkbit service status check: %1 (active=%2)").arg(status).arg(isActive).toUtf8().constData());
    qDebug() << "Hawkbit service status:" << status << "Active:" << isActive;
    emit hawkbitServiceStatusChanged(isActive);
    
    return isActive;
}

void SystemInfo::stopHawkbitUpdater() {
    DLT_LOG_SYS_INFO("F2 Button pressed - Stopping Hawkbit updater service...");
    qDebug() << "Stopping Hawkbit updater service...";
    
    QProcess process;
    process.start("systemctl", QStringList() << "stop" << "rauc-hawkbit-cpp.service");
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        DLT_LOG_SYS_INFO("Hawkbit updater service stopped successfully");
        qDebug() << "Hawkbit updater service stopped successfully";
        emit hawkbitServiceStatusChanged(false);
    } else {
        QString error = process.readAllStandardError();
        DLT_LOG_SYS_WARN(QString("Failed to stop Hawkbit service: %1").arg(error).toUtf8().constData());
        qDebug() << "Failed to stop Hawkbit updater service:" << error;
    }
    
    // Clean up signal file
    DLT_LOG_SYS_INFO("Removing start signal file");
    QFile::remove("/tmp/rauc-hawkbit-start-signal");
}

QString SystemInfo::getHawkbitServiceLogs(int lines) {
    QProcess process;
    process.start("journalctl", QStringList() << "-u" << "rauc-hawkbit-cpp.service" << "-n" << QString::number(lines) << "--no-pager");
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        QString logs = process.readAllStandardOutput();
        
        // Log key events found in service logs
        if (logs.contains("Connected to server", Qt::CaseInsensitive)) {
            DLT_LOG_SYS_INFO("Hawkbit service connected to server");
        }
        if (logs.contains("Deployment found", Qt::CaseInsensitive)) {
            DLT_LOG_SYS_INFO("Hawkbit deployment found");
        }
        if (logs.contains("rauc install", Qt::CaseInsensitive)) {
            DLT_LOG_SYS_INFO("Hawkbit triggered RAUC installation");
        }
        if (logs.contains("error", Qt::CaseInsensitive)) {
            DLT_LOG_SYS_WARN("Hawkbit service reported errors - check journalctl");
        }
        
        return logs;
    } else {
        DLT_LOG_SYS_ERROR("Failed to retrieve Hawkbit service logs");
        return "Failed to retrieve service logs";
    }
}

void SystemInfo::logUIEvent(const QString &event, const QString &details) {
    QString logMessage = QString("UI Event: %1").arg(event);
    if (!details.isEmpty()) {
        logMessage += QString(" - %1").arg(details);
    }
    DLT_LOG_SYS_INFO(logMessage.toUtf8().constData());
}

QString SystemInfo::getHawkbitServiceStatus() {
    QProcess statusProcess;
    statusProcess.start("systemctl", QStringList() << "status" << "rauc-hawkbit-cpp.service" << "--no-pager");
    statusProcess.waitForFinished(5000);
    
    QString output = statusProcess.readAllStandardOutput();
    QString error = statusProcess.readAllStandardError();
    
    QString fullStatus = "=== Service Status ===\n" + output;
    if (!error.isEmpty()) {
        fullStatus += "\n=== Errors ===\n" + error;
    }
    
    DLT_LOG_SYS_INFO("Hawkbit service detailed status requested");
    return fullStatus;
}

QString SystemInfo::checkHawkbitConfiguration() {
    QString configInfo = "=== Hawkbit Configuration Check ===\n";
    
    // Check configuration file
    QFile configFile("/etc/rauc-hawkbit-cpp/config.json");
    if (configFile.exists()) {
        if (configFile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&configFile);
            QString config = stream.readAll();
            configInfo += "Configuration file found:\n" + config + "\n\n";
            configFile.close();
            
            // Parse for server URL
            if (config.contains("hawkbit_server")) {
                DLT_LOG_SYS_INFO("Hawkbit configuration file found and contains server URL");
            } else {
                DLT_LOG_SYS_WARN("Hawkbit configuration file found but no server URL detected");
            }
        } else {
            configInfo += "Configuration file exists but cannot be read\n\n";
            DLT_LOG_SYS_ERROR("Cannot read Hawkbit configuration file");
        }
    } else {
        configInfo += "Configuration file not found: /etc/rauc-hawkbit-cpp/config.json\n\n";
        DLT_LOG_SYS_ERROR("Hawkbit configuration file not found");
    }
    
    // Check signal file
    QFile signalFile("/tmp/rauc-hawkbit-start-signal");
    if (signalFile.exists()) {
        configInfo += "Start signal file exists: YES\n";
        DLT_LOG_SYS_INFO("Hawkbit start signal file exists");
    } else {
        configInfo += "Start signal file exists: NO\n";
        DLT_LOG_SYS_WARN("Hawkbit start signal file missing");
    }
    
    // Check RAUC status
    QProcess raucProcess;
    raucProcess.start("rauc", QStringList() << "status");
    raucProcess.waitForFinished(3000);
    
    if (raucProcess.exitCode() == 0) {
        configInfo += "RAUC is accessible: YES\n";
        DLT_LOG_SYS_INFO("RAUC service is accessible");
    } else {
        configInfo += "RAUC is accessible: NO\n";
        configInfo += "RAUC Error: " + raucProcess.readAllStandardError() + "\n";
        DLT_LOG_SYS_ERROR("RAUC service is not accessible");
    }
    
    return configInfo;
}

bool SystemInfo::testNetworkConnectivity() {
    DLT_LOG_SYS_INFO("Testing network connectivity for Hawkbit");
    
    // First check if Hawkbit server is configured
    QString config = checkHawkbitConfiguration();
    QString hawkbitServer;
    
    // Extract server from config
    QStringList lines = config.split('\n');
    for (const QString &line : lines) {
        if (line.contains("hawkbit_server")) {
            QStringList parts = line.split(':', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                hawkbitServer = parts[1].trimmed();
                // Remove http:// or https:// prefix for ping test
                hawkbitServer = hawkbitServer.replace("http://", "").replace("https://", "");
                // Remove port number if present
                if (hawkbitServer.contains(':')) {
                    hawkbitServer = hawkbitServer.split(':')[0];
                }
                break;
            }
        }
    }
    
    QString testTarget = hawkbitServer.isEmpty() ? "8.8.8.8" : hawkbitServer;
    DLT_LOG_SYS_INFO(QString("Testing connectivity to: %1").arg(testTarget).toUtf8().constData());
    
    // Test connectivity to Hawkbit server or fallback to Google DNS
    QProcess pingProcess;
    pingProcess.start("ping", QStringList() << "-c" << "1" << "-W" << "3" << testTarget);
    pingProcess.waitForFinished(5000);
    
    if (pingProcess.exitCode() == 0) {
        if (hawkbitServer.isEmpty()) {
            DLT_LOG_SYS_INFO("Network connectivity test: PASSED (can reach internet)");
            DLT_LOG_SYS_WARN("Network available but no Hawkbit server configured");
        } else {
            DLT_LOG_SYS_INFO(QString("Network connectivity test: PASSED (can reach Hawkbit server: %1)").arg(hawkbitServer).toUtf8().constData());
            DLT_LOG_SYS_INFO("Network available and Hawkbit server reachable");
        }
        return true;
    } else {
        DLT_LOG_SYS_ERROR(QString("Network connectivity test: FAILED (cannot reach %1)").arg(testTarget).toUtf8().constData());
        return false;
    }
}

void SystemInfo::updateSoftwareVersion()
{
    QString versionFile = "/etc/sw-version";
    QString newSoftwareVersion = readFileContent(versionFile).trimmed();
    
    if (newSoftwareVersion.isEmpty()) {
        newSoftwareVersion = "Unknown";
    }
    
    if (m_softwareVersion != newSoftwareVersion) {
        m_softwareVersion = newSoftwareVersion;
        emit softwareVersionChanged();
        DLT_LOG_SYS_INFO(QString("Software version updated: %1").arg(m_softwareVersion).toUtf8().constData());
    }
}