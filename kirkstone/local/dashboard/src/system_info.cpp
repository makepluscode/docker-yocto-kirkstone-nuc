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
    , m_lastCpuTotal(0)
    , m_lastCpuIdle(0)
{
    // Initialize system details that don't change frequently
    updateSystemDetails();
    updateBuildInfo();
    
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

void SystemInfo::rebootSystem() {
    QProcess::startDetached("reboot");
} 

void SystemInfo::refresh() {
    updateSystemInfo();
}

void SystemInfo::bootToSlotA() {
    qDebug() << "Booting to Slot A...";
    
    // Check if slot A is healthy
    bool slotAHealthy = isSlotAHealthy();
    QString slotAStatus = getSlotAStatus();
    
    qDebug() << "Slot A status:" << slotAStatus << "Healthy:" << slotAHealthy;
    
    // Set GRUB environment to boot to slot A first
    QProcess process;
    process.start("grub-editenv", QStringList() << "/grubenv/grubenv" << "set" << "ORDER=A B");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        qDebug() << "Successfully set boot order to A B";
        
        // Mark slot A as good
        QProcess markProcess;
        markProcess.start("grub-editenv", QStringList() << "/grubenv/grubenv" << "set" << "A_OK=1");
        markProcess.waitForFinished();
        qDebug() << "Marked slot A as good";
        
        // Create a temporary GRUB configuration for slot A
        QProcess grubProcess;
        grubProcess.start("sh", QStringList() << "-c" << "echo 'linux /bzImage LABEL=boot root=/dev/sda2 rootwait console=ttyS0,115200 console=tty0' > /tmp/grub-slot-a.cfg");
        grubProcess.waitForFinished();
        
        // Reboot the system
        QProcess::startDetached("reboot");
    } else {
        qDebug() << "Failed to set boot order:" << process.errorString();
    }
}

void SystemInfo::bootToSlotB() {
    qDebug() << "Booting to Slot B...";
    
    // Check if slot B is healthy
    bool slotBHealthy = isSlotBHealthy();
    QString slotBStatus = getSlotBStatus();
    
    qDebug() << "Slot B status:" << slotBStatus << "Healthy:" << slotBHealthy;
    
    // Set GRUB environment to boot to slot B first
    QProcess process;
    process.start("grub-editenv", QStringList() << "/grubenv/grubenv" << "set" << "ORDER=B A");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        qDebug() << "Successfully set boot order to B A";
        
        // Mark slot B as good
        QProcess markProcess;
        markProcess.start("grub-editenv", QStringList() << "/grubenv/grubenv" << "set" << "B_OK=1");
        markProcess.waitForFinished();
        qDebug() << "Marked slot B as good";
        
        // Create a temporary GRUB configuration for slot B
        QProcess grubProcess;
        grubProcess.start("sh", QStringList() << "-c" << "echo 'linux /bzImage LABEL=boot root=/dev/sda3 rootwait console=ttyS0,115200 console=tty0' > /tmp/grub-slot-b.cfg");
        grubProcess.waitForFinished();
        
        // Reboot the system
        QProcess::startDetached("reboot");
    } else {
        qDebug() << "Failed to set boot order:" << process.errorString();
    }
}

QString SystemInfo::getCurrentBootSlot() {
    // Check which slot is currently booted by examining the root device
    QProcess process;
    process.start("readlink", QStringList() << "-f" << "/dev/root");
    process.waitForFinished();
    
    QString rootDevice = process.readAllStandardOutput().trimmed();
    qDebug() << "Current root device:" << rootDevice;
    
    if (rootDevice.contains("sda2")) {
        return "A";
    } else if (rootDevice.contains("sda3")) {
        return "B";
    } else {
        return "Unknown";
    }
}

QString SystemInfo::getBootOrder() {
    QProcess process;
    process.start("grub-editenv", QStringList() << "/grubenv/grubenv" << "list");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString &line : lines) {
        if (line.startsWith("ORDER=")) {
            return line.mid(6); // Remove "ORDER=" prefix
        }
    }
    
    return "Unknown";
}

bool SystemInfo::isSlotAHealthy() {
    QProcess process;
    process.start("rauc", QStringList() << "status");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    return output.contains("bootname: A") && output.contains("boot status: good");
}

bool SystemInfo::isSlotBHealthy() {
    QProcess process;
    process.start("rauc", QStringList() << "status");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    return output.contains("bootname: B") && output.contains("boot status: good");
}

QString SystemInfo::getSlotAStatus() {
    QProcess process;
    process.start("rauc", QStringList() << "status");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString &line : lines) {
        if (line.contains("bootname: A")) {
            // Find the next line with boot status
            int index = lines.indexOf(line);
            if (index + 1 < lines.size()) {
                QString statusLine = lines[index + 1];
                if (statusLine.contains("boot status:")) {
                    return statusLine.split("boot status:").last().trimmed();
                }
            }
        }
    }
    
    return "unknown";
}

QString SystemInfo::getSlotBStatus() {
    QProcess process;
    process.start("rauc", QStringList() << "status");
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString &line : lines) {
        if (line.contains("bootname: B")) {
            // Find the next line with boot status
            int index = lines.indexOf(line);
            if (index + 1 < lines.size()) {
                QString statusLine = lines[index + 1];
                if (statusLine.contains("boot status:")) {
                    return statusLine.split("boot status:").last().trimmed();
                }
            }
        }
    }
    
    return "unknown";
} 