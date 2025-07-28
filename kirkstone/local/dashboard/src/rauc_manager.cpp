#include "rauc_manager.h"
#include <QTextStream>
#include <QDebug>
#include <dlt/dlt.h>

DltContext RaucManager::m_ctx;

static void ensureDltContext()
{
    static bool initialized = false;
    if (!initialized) {
        DLT_REGISTER_CONTEXT(RaucManager::m_ctx, "RUC", "RAUC Manager");
        initialized = true;
    }
}

RaucManager::RaucManager(QObject *parent) : QObject(parent) {}

// Helper macro to log info with DLT
#define RUC_LOG(str)                              \
    do {                                         \
        ensureDltContext();                      \
        DLT_LOG(RaucManager::m_ctx,              \
                DLT_LOG_INFO,                    \
                DLT_STRING(str));                \
    } while (0)

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

void RaucManager::refresh() {
    RUC_LOG("Refresh status requested");
    runProcess("/usr/bin/rauc", {"status"});
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