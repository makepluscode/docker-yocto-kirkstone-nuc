#include "rauc_manager.h"
#include <QTextStream>
#include <QDebug>
#include <dlt/dlt.h>

DltContext RaucManager::m_ctx;

static void ensureDltContext()
{
    static bool initialized = false;
    if (!initialized) {
        DLT_REGISTER_CONTEXT(RaucManager::m_ctx, "RAUM", "RAUC Manager");
        initialized = true;
    }
}

// Helper macro to log info with DLT
#define RUC_LOG(str)                              \
    do {                                         \
        ensureDltContext();                      \
        DLT_LOG(RaucManager::m_ctx,              \
                DLT_LOG_INFO,                    \
                DLT_STRING(str));                \
    } while (0)

RaucManager::RaucManager(QObject *parent) : QObject(parent) {}

void RaucManager::runProcess(const QString &cmd, const QStringList &args) {
    RUC_LOG(QString("Run process: %1 %2").arg(cmd, args.join(" ")).toUtf8().constData());
    QProcess proc;
    proc.start(cmd, args);
    proc.waitForFinished(4000);
    updateStatus(QString::fromUtf8(proc.readAllStandardOutput()));
}

void RaucManager::updateStatus(const QString &output) {
    m_status = output.trimmed();
    parseStatus(output);
    emit statusChanged();
}

void RaucManager::parseStatus(const QString &output) {
    // Try to parse as JSON first
    if (output.trimmed().startsWith("{")) {
        parseJsonStatus(output);
    } else {
        // Fallback to old text parsing
        m_bootSlot.clear();
        m_activatedSlot.clear();
        const auto lines = output.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith("Booted from:")) {
                // e.g., "Booted from: rootfs.0 (/dev/sda2)"
                m_bootSlot = line.section(':',1).simplified().section(' ',0,0);
            } else if (line.startsWith("Activated:")) {
                m_activatedSlot = line.section(':',1).simplified().section(' ',0,0);
            }
        }
    }
}

void RaucManager::parseJsonStatus(const QString &jsonOutput) {
    RUC_LOG(QString("Parsing JSON output: %1").arg(jsonOutput).toUtf8().constData());
    // Parse JSON using regex patterns
    QRegExp compatiblePattern("\"compatible\":\"([^\"]+)\"");
    QRegExp variantPattern("\"variant\":\"([^\"]*)\"");
    QRegExp bootedPattern("\"booted\":\"([^\"]+)\"");
    QRegExp bootPrimaryPattern("\"boot_primary\":([^,}]+)");

    // Parse compatible
    if (compatiblePattern.indexIn(jsonOutput) != -1) {
        m_compatible = compatiblePattern.cap(1);
    }

    // Parse variant
    if (variantPattern.indexIn(jsonOutput) != -1) {
        m_variant = variantPattern.cap(1);
    }

    // Parse booted
    if (bootedPattern.indexIn(jsonOutput) != -1) {
        m_booted = bootedPattern.cap(1);
    }

    // Parse boot_primary
    if (bootPrimaryPattern.indexIn(jsonOutput) != -1) {
        QString bootPrimary = bootPrimaryPattern.cap(1).trimmed();
        if (bootPrimary != "null") {
            m_bootPrimary = bootPrimary;
        }
    }

    // Parse slots using regex - updated for actual JSON structure
    // The slots are in an array, so we need to find them within the array
    // More flexible patterns that can handle the compact JSON format
    // First find the slots array and then parse each slot
    QRegExp slotsArrayPattern("\"slots\":\\s*\\[([^\\]]+)\\]");
    if (slotsArrayPattern.indexIn(jsonOutput) != -1) {
        QString slotsArray = slotsArrayPattern.cap(1);
        RUC_LOG(QString("Found slots array: %1").arg(slotsArray).toUtf8().constData());

        // Parse slot A (rootfs.0) - search in the entire JSON for the slot
        QRegExp slotAPattern("\"rootfs\\.0\":\\s*\\{[^}]*\"state\":\"([^\"]+)\"[^}]*\"boot_status\":\"([^\"]+)\"[^}]*\"device\":\"([^\"]+)\"[^}]*\"bootname\":\"([^\"]+)\"");
        // Parse slot B (rootfs.1) - search in the entire JSON for the slot
        QRegExp slotBPattern("\"rootfs\\.1\":\\s*\\{[^}]*\"state\":\"([^\"]+)\"[^}]*\"boot_status\":\"([^\"]+)\"[^}]*\"device\":\"([^\"]+)\"[^}]*\"bootname\":\"([^\"]+)\"");

        // Parse slot A
        if (slotAPattern.indexIn(jsonOutput) != -1) {
            m_slotAState = slotAPattern.cap(1);
            m_slotAStatus = slotAPattern.cap(2);
            m_slotADevice = slotAPattern.cap(3);
            QString bootname = slotAPattern.cap(4);

            RUC_LOG(QString("Slot A parsed - State: %1, Status: %2, Device: %3, Bootname: %4")
                    .arg(m_slotAState).arg(m_slotAStatus).arg(m_slotADevice).arg(bootname).toUtf8().constData());

            if (m_slotAState == "booted") {
                m_bootSlot = "rootfs.0";
            }
            if (m_slotAState == "active") {
                m_activatedSlot = "rootfs.0";
            }
        } else {
            RUC_LOG("Failed to parse Slot A");
        }

        // Parse slot B
        if (slotBPattern.indexIn(jsonOutput) != -1) {
            m_slotBState = slotBPattern.cap(1);
            m_slotBStatus = slotBPattern.cap(2);
            m_slotBDevice = slotBPattern.cap(3);
            QString bootname = slotBPattern.cap(4);

            RUC_LOG(QString("Slot B parsed - State: %1, Status: %2, Device: %3, Bootname: %4")
                    .arg(m_slotBState).arg(m_slotBStatus).arg(m_slotBDevice).arg(bootname).toUtf8().constData());

            if (m_slotBState == "booted") {
                m_bootSlot = "rootfs.1";
            }
            if (m_slotBState == "active") {
                m_activatedSlot = "rootfs.1";
            }
        } else {
            RUC_LOG("Failed to parse Slot B");
        }
    } else {
        RUC_LOG("Failed to find slots array in JSON");
    }

    // Update status text for backward compatibility
    m_status = QString("Compatible: %1\nBooted: %2\nSlot A: %3 (%4)\nSlot B: %5 (%6)")
               .arg(m_compatible)
               .arg(m_booted)
               .arg(m_slotAState)
               .arg(m_slotAStatus)
               .arg(m_slotBState)
               .arg(m_slotBStatus);
}

void RaucManager::refresh() {
    RUC_LOG("Refresh status requested");
    runProcess("/usr/bin/rauc", {"status", "--output-format=json"});
}

static void setBootOrder(const QString &order) {
    QProcess::execute("/usr/bin/grub-editenv", {"/grubenv/grubenv", "set", QString("ORDER=%1").arg(order)});
}

void RaucManager::bootSlotA() {
    RUC_LOG("Boot Slot A button pressed");
    setBootOrder("A B");
    refresh();
}

void RaucManager::bootSlotB() {
    RUC_LOG("Boot Slot B button pressed");
    setBootOrder("B A");
    refresh();
}

#undef RUC_LOG
