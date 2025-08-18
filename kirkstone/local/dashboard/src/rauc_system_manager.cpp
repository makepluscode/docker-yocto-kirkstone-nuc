#include "rauc_system_manager.h"
#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>

// Constants
const QString RaucSystemManager::RAUC_BUNDLE_PATH = "/data/nuc-image-qt5-bundle-intel-corei7-64.raucb";
const QString RaucSystemManager::GRUB_CONFIG_PATH = "/grubenv/grubenv";

// DLT context definition
DltContext RaucSystemManager::m_ctx;

// Helper macro to log info with DLT
#define DLT_LOG_CXX_INFO(str) \
    do { \
        DLT_LOG(RaucSystemManager::m_ctx,     \
                DLT_LOG_INFO,                 \
                DLT_STRING(str));             \
    } while(0)

#define DLT_LOG_CXX_WARN(str) \
    do { \
        DLT_LOG(RaucSystemManager::m_ctx,     \
                DLT_LOG_WARN,                 \
                DLT_STRING(str));             \
    } while(0)

#define DLT_LOG_CXX_ERROR(str) \
    do { \
        DLT_LOG(RaucSystemManager::m_ctx,     \
                DLT_LOG_ERROR,                \
                DLT_STRING(str));             \
    } while(0)

RaucSystemManager::RaucSystemManager(QObject *parent)
    : QObject(parent)
    , m_currentBootSlot("Unknown")
    , m_bootOrder("Unknown")
    , m_slotAStatus("unknown")
    , m_slotBStatus("unknown")
    , m_updateInProgress(false)
    , m_bundleExists(false)
    , m_bundlePath("")
    , m_bundleSize(0)
    , m_bundleSizeFormatted("0 B")
    , m_bundleModified("")
    , m_raucProcess(nullptr)
{
    // Register DLT context (once per process)
    static bool contextRegistered = false;
    if (!contextRegistered) {
        DLT_REGISTER_CONTEXT(RaucSystemManager::m_ctx, "RAUS", "RAUC System Manager");
        contextRegistered = true;
    }

    DLT_LOG_CXX_INFO("RaucSystemManager initialized");

    // Setup status refresh timer
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &RaucSystemManager::refreshStatus);
    m_statusTimer->start(5000); // Refresh every 5 seconds

    // Setup D-Bus monitoring timer for RAUC operations
    m_dbusMonitorTimer = new QTimer(this);
    connect(m_dbusMonitorTimer, &QTimer::timeout, this, &RaucSystemManager::checkRaucDBusProgress);

    // Initial status update
    refreshStatus();
}

void RaucSystemManager::refreshStatus()
{
    updateCurrentBootSlot();
    updateBootOrder();
    updateSlotStatus();
    updateBundleInfo();
}

void RaucSystemManager::updateCurrentBootSlot()
{
    QString raucOutput = executeRaucCommand(QStringList() << "status");
    QStringList lines = raucOutput.split('\n');

    QString newSlot = "Unknown";

    for (const QString &line : lines) {
        if (line.contains("Booted from:")) {
            if (line.contains("rootfs.0")) {
                newSlot = "rootfs.0";
            } else if (line.contains("rootfs.1")) {
                newSlot = "rootfs.1";
            }
            break;
        }
    }

    if (m_currentBootSlot != newSlot) {
        m_currentBootSlot = newSlot;
        emit currentBootSlotChanged();
        DLT_LOG(m_ctx, DLT_LOG_INFO, DLT_STRING("Current boot slot updated: "), DLT_STRING(m_currentBootSlot.toUtf8().constData()));
    }
}

void RaucSystemManager::updateBootOrder()
{
    QProcess process;
    process.start("grub-editenv", QStringList() << GRUB_CONFIG_PATH << "list");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');

    QString newOrder = "Unknown";
    for (const QString &line : lines) {
        if (line.startsWith("ORDER=")) {
            newOrder = line.mid(6); // Remove "ORDER=" prefix
            break;
        }
    }

    if (m_bootOrder != newOrder) {
        m_bootOrder = newOrder;
        emit bootOrderChanged();
    }
}

void RaucSystemManager::updateSlotStatus()
{
    QString raucOutput = executeRaucCommand(QStringList() << "status");
    QStringList lines = raucOutput.split('\n');

    QString newSlotAStatus = "unknown";
    QString newSlotBStatus = "unknown";

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];

        if (line.contains("bootname: A")) {
            // Look for boot status in the following lines
            for (int j = i + 1; j < lines.size() && j < i + 5; ++j) {
                if (lines[j].contains("boot status:")) {
                    newSlotAStatus = lines[j].split("boot status:").last().trimmed();
                    break;
                }
            }
        } else if (line.contains("bootname: B")) {
            // Look for boot status in the following lines
            for (int j = i + 1; j < lines.size() && j < i + 5; ++j) {
                if (lines[j].contains("boot status:")) {
                    newSlotBStatus = lines[j].split("boot status:").last().trimmed();
                    break;
                }
            }
        }
    }

    if (m_slotAStatus != newSlotAStatus) {
        m_slotAStatus = newSlotAStatus;
        emit slotAStatusChanged();
        emit slotAHealthyChanged();
    }

    if (m_slotBStatus != newSlotBStatus) {
        m_slotBStatus = newSlotBStatus;
        emit slotBStatusChanged();
        emit slotBHealthyChanged();
    }
}

void RaucSystemManager::bootToSlotA()
{
    qDebug() << "Booting to Slot A...";
    qDebug() << "Slot A status:" << m_slotAStatus << "Healthy:" << slotAHealthy();

    // Set GRUB environment to boot to slot A first
    executeGrubScript("ORDER=A B");

    // Mark slot A as good
    executeGrubScript("A_OK=1");

    qDebug() << "Successfully set boot order to A B and marked slot A as good";

    // Reboot the system
    rebootSystem();
}

void RaucSystemManager::bootToSlotB()
{
    qDebug() << "Booting to Slot B...";
    qDebug() << "Slot B status:" << m_slotBStatus << "Healthy:" << slotBHealthy();

    // Set GRUB environment to boot to slot B first
    executeGrubScript("ORDER=B A");

    // Mark slot B as good
    executeGrubScript("B_OK=1");

    qDebug() << "Successfully set boot order to B A and marked slot B as good";

    // Reboot the system
    rebootSystem();
}

bool RaucSystemManager::checkRaucBundle()
{
    DLT_LOG_CXX_INFO("Checking for RAUC bundle...");
    qDebug() << "Checking for RAUC bundle...";

    QFileInfo bundleFile(RAUC_BUNDLE_PATH);
    bool exists = bundleFile.exists();

    QString logMsg = QString("RAUC bundle exists: %1").arg(exists ? "YES" : "NO");
    DLT_LOG_CXX_INFO(logMsg.toUtf8().constData());
    qDebug() << "RAUC bundle exists:" << exists;

    if (exists) {
        QString sizeMsg = QString("Bundle size: %1 bytes").arg(bundleFile.size());
        DLT_LOG_CXX_INFO(sizeMsg.toUtf8().constData());
        qDebug() << "Bundle size:" << bundleFile.size() << "bytes";
    }

    return exists;
}

void RaucSystemManager::installRaucBundle()
{
    DLT_LOG_CXX_INFO("Starting RAUC bundle installation...");
    qDebug() << "Installing RAUC bundle...";

    if (!checkRaucBundle()) {
        DLT_LOG_CXX_ERROR("RAUC bundle not found, cannot install");
        qDebug() << "RAUC bundle not found, cannot install";
        emit updateCompleted(false);
        return;
    }

    if (m_updateInProgress) {
        DLT_LOG_CXX_WARN("Update already in progress");
        qDebug() << "Update already in progress";
        return;
    }

    setUpdateInProgress(true);
    DLT_LOG_CXX_INFO("Update progress: Starting RAUC installation...");
    emit updateProgress(10, "Starting RAUC installation...");

    // Create new process for RAUC installation
    if (m_raucProcess) {
        m_raucProcess->deleteLater();
    }

    m_raucProcess = new QProcess(this);
    connect(m_raucProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RaucSystemManager::onRaucProcessFinished);
    connect(m_raucProcess, &QProcess::errorOccurred,
            this, &RaucSystemManager::onRaucProcessError);

    // Start RAUC install command
    QString cmdMsg = QString("Executing: rauc install %1").arg(RAUC_BUNDLE_PATH);
    DLT_LOG_CXX_INFO(cmdMsg.toUtf8().constData());

    m_raucProcess->start("rauc", QStringList() << "install" << RAUC_BUNDLE_PATH);

    if (!m_raucProcess->waitForStarted()) {
        DLT_LOG_CXX_ERROR("Failed to start RAUC installation process");
        qDebug() << "Failed to start RAUC installation process";
        setUpdateInProgress(false);
        emit updateCompleted(false);
    } else {
        DLT_LOG_CXX_INFO("RAUC installation process started successfully");
        emit updateProgress(25, "RAUC installation started...");
    }
}

void RaucSystemManager::startSoftwareUpdate()
{
    DLT_LOG_CXX_INFO("Starting software update process...");
    qDebug() << "Starting software update...";

    emit updateProgress(5, "Checking for update bundle...");

    if (checkRaucBundle()) {
        DLT_LOG_CXX_INFO("Bundle found, proceeding with installation");
        installRaucBundle();
    } else {
        DLT_LOG_CXX_WARN("No RAUC bundle found for update");
        qDebug() << "No RAUC bundle found for update";

        // Check what files are actually in /data
        QDir dataDir("/data");
        QStringList files = dataDir.entryList(QDir::Files);
        QString fileList = files.join(", ");
        QString debugMsg = QString("Files in /data: [%1]").arg(fileList.isEmpty() ? "none" : fileList);
        DLT_LOG_CXX_INFO(debugMsg.toUtf8().constData());
        qDebug() << "Files in /data:" << files;

        emit updateProgress(100, QString("No update bundle found in /data/\n\nFiles present: %1\n\nPlace a .raucb file in /data/ directory.").arg(fileList.isEmpty() ? "none" : fileList));
        emit updateCompleted(false);
    }
}

void RaucSystemManager::rebootSystem()
{
    DLT_LOG_CXX_INFO("Rebooting system...");
    qDebug() << "Rebooting system...";
    QProcess::startDetached("reboot", QStringList());
}

void RaucSystemManager::onRaucProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString finishMsg = QString("RAUC installation finished with exit code: %1").arg(exitCode);
    DLT_LOG_CXX_INFO(finishMsg.toUtf8().constData());
    qDebug() << "RAUC installation finished with exit code:" << exitCode;

    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        DLT_LOG_CXX_INFO("RAUC installation completed successfully");
        qDebug() << "RAUC installation successful";
        emit updateProgress(100, "Installation completed successfully");
        emit updateCompleted(true);

        // Refresh status after successful installation
        refreshStatus();
    } else {
        DLT_LOG_CXX_ERROR("RAUC installation failed");
        qDebug() << "RAUC installation failed";
        emit updateProgress(100, "Installation failed");
        emit updateCompleted(false);
    }

    setUpdateInProgress(false);

    if (m_raucProcess) {
        m_raucProcess->deleteLater();
        m_raucProcess = nullptr;
    }
}

void RaucSystemManager::onRaucProcessError(QProcess::ProcessError error)
{
    QString errorMsg = QString("RAUC process error: %1").arg(error);
    DLT_LOG_CXX_ERROR(errorMsg.toUtf8().constData());
    qDebug() << "RAUC process error:" << error;

    emit updateProgress(100, "Process error occurred");
    emit updateCompleted(false);
    setUpdateInProgress(false);

    if (m_raucProcess) {
        m_raucProcess->deleteLater();
        m_raucProcess = nullptr;
    }
}

void RaucSystemManager::executeGrubScript(const QString &script)
{
    QStringList parts = script.split('=');
    if (parts.size() == 2) {
        QProcess process;
        process.start("grub-editenv", QStringList() << GRUB_CONFIG_PATH << "set" << script);
        process.waitForFinished();

        if (process.exitCode() != 0) {
            qDebug() << "Failed to execute GRUB script:" << script << process.errorString();
        }
    }
}

QString RaucSystemManager::executeRaucCommand(const QStringList &arguments)
{
    QProcess process;
    process.start("rauc", arguments);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        return process.readAllStandardOutput();
    } else {
        qDebug() << "RAUC command failed:" << arguments << process.errorString();
        return QString();
    }
}

void RaucSystemManager::setUpdateInProgress(bool inProgress)
{
    if (m_updateInProgress != inProgress) {
        m_updateInProgress = inProgress;
        emit updateInProgressChanged();
    }
}

void RaucSystemManager::updateBundleInfo()
{
    // Check for RAUC bundle files in /data directory
    QDir dataDir("/data");
    QStringList filters;
    filters << "*.raucb";
    dataDir.setNameFilters(filters);
    QFileInfoList bundleFiles = dataDir.entryInfoList(QDir::Files | QDir::Readable);

    bool bundleExists = false;
    QString bundlePath = "";
    qint64 bundleSize = 0;
    QString bundleSizeFormatted = "0 B";
    QString bundleModified = "";

    if (!bundleFiles.isEmpty()) {
        // Use the first .raucb file found
        QFileInfo bundleInfo = bundleFiles.first();
        bundleExists = true;
        bundlePath = bundleInfo.absoluteFilePath();
        bundleSize = bundleInfo.size();
        bundleSizeFormatted = formatBytes(bundleSize);
        bundleModified = bundleInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
    }

    // Update properties if changed
    if (m_bundleExists != bundleExists) {
        m_bundleExists = bundleExists;
        emit bundleExistsChanged();
    }

    if (m_bundlePath != bundlePath) {
        m_bundlePath = bundlePath;
        emit bundlePathChanged();
    }

    if (m_bundleSize != bundleSize) {
        m_bundleSize = bundleSize;
        m_bundleSizeFormatted = bundleSizeFormatted;
        emit bundleSizeChanged();
    }

    if (m_bundleModified != bundleModified) {
        m_bundleModified = bundleModified;
        emit bundleModifiedChanged();
    }
}

QString RaucSystemManager::formatBytes(qint64 bytes)
{
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int suffixIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && suffixIndex < 4) {
        size /= 1024.0;
        suffixIndex++;
    }

    if (suffixIndex == 0) {
        return QString("%1 %2").arg(static_cast<int>(size)).arg(suffixes[suffixIndex]);
    } else {
        return QString("%1 %2").arg(size, 0, 'f', 1).arg(suffixes[suffixIndex]);
    }
}

void RaucSystemManager::monitorRaucDBus()
{
    DLT_LOG_CXX_INFO("Starting RAUC D-Bus monitoring for Hawkbit updates");
    qDebug() << "Starting RAUC D-Bus monitoring for Hawkbit updates";

    // Start monitoring timer (every 2 seconds)
    m_dbusMonitorTimer->start(2000);
}

bool RaucSystemManager::isRaucInstallationRunning()
{
    // Check if RAUC is currently installing by querying D-Bus
    QProcess process;
    process.start("gdbus", QStringList()
        << "call" << "--system"
        << "--dest" << "de.pengutronix.rauc"
        << "--object-path" << "/de/pengutronix/rauc/Installer"
        << "--method" << "org.freedesktop.DBus.Properties.Get"
        << "de.pengutronix.rauc.Installer" << "Operation");
    process.waitForFinished(3000);

    QString output = process.readAllStandardOutput().trimmed();
    qDebug() << "RAUC Operation status:" << output;

    // If RAUC is installing, output will contain 'installing' or 'active'
    return output.contains("installing", Qt::CaseInsensitive) ||
           output.contains("active", Qt::CaseInsensitive);
}

void RaucSystemManager::checkRaucDBusProgress()
{
    if (!isRaucInstallationRunning()) {
        // No installation running, stop monitoring
        m_dbusMonitorTimer->stop();
        return;
    }

    // Get RAUC installation progress via D-Bus
    QProcess progressProcess;
    progressProcess.start("gdbus", QStringList()
        << "call" << "--system"
        << "--dest" << "de.pengutronix.rauc"
        << "--object-path" << "/de/pengutronix/rauc/Installer"
        << "--method" << "org.freedesktop.DBus.Properties.Get"
        << "de.pengutronix.rauc.Installer" << "Progress");
    progressProcess.waitForFinished(3000);

    QString progressOutput = progressProcess.readAllStandardOutput().trimmed();

    // Get RAUC last error (if any)
    QProcess errorProcess;
    errorProcess.start("gdbus", QStringList()
        << "call" << "--system"
        << "--dest" << "de.pengutronix.rauc"
        << "--object-path" << "/de/pengutronix/rauc/Installer"
        << "--method" << "org.freedesktop.DBus.Properties.Get"
        << "de.pengutronix.rauc.Installer" << "LastError");
    errorProcess.waitForFinished(3000);

    QString errorOutput = errorProcess.readAllStandardOutput().trimmed();

    qDebug() << "RAUC Progress:" << progressOutput;
    qDebug() << "RAUC LastError:" << errorOutput;

    // Parse progress and emit signals
    if (progressOutput.contains("(")) {
        // Extract percentage from D-Bus output format: "(<int32 percentage>, '<string message>')"
        QString cleaned = progressOutput;
        cleaned = cleaned.remove(QRegExp("[()']")).split(',').first();

        bool ok;
        int percentage = cleaned.toInt(&ok);

        if (ok && percentage >= 0) {
            QString message = "RAUC installation in progress via Hawkbit...";

            // Extract message if available
            if (progressOutput.contains("'") && progressOutput.split("'").size() > 1) {
                message = progressOutput.split("'").at(1);
            }

            if (percentage < 100) {
                DLT_LOG_CXX_INFO(QString("RAUC Progress: %1% - %2").arg(percentage).arg(message).toUtf8().constData());
                emit updateProgress(percentage, QString("RAUC Hawkbit: %1").arg(message));
            } else {
                // Installation completed
                DLT_LOG_CXX_INFO("RAUC installation completed via Hawkbit");
                emit updateProgress(100, "RAUC installation completed via Hawkbit!");
                emit updateCompleted(true);
                m_dbusMonitorTimer->stop();
            }
        }
    }

    // Check for errors
    if (!errorOutput.isEmpty() && !errorOutput.contains("''") && !errorOutput.contains("()")) {
        DLT_LOG_CXX_ERROR(QString("RAUC installation error: %1").arg(errorOutput).toUtf8().constData());
        emit updateProgress(0, QString("RAUC error: %1").arg(errorOutput));
        emit updateCompleted(false);
        m_dbusMonitorTimer->stop();
    }
}
