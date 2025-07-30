#include "grub_manager.h"
#include <QTextStream>
#include <QDebug>
#include <dlt/dlt.h>

DltContext GrubManager::m_ctx;

static void ensureDltContext()
{
    static bool initialized = false;
    if (!initialized) {
        DLT_REGISTER_CONTEXT(GrubManager::m_ctx, "GRB", "GRUB Manager");
        initialized = true;
    }
}

// Helper macro to log info with DLT
#define GRB_LOG(str)                              \
    do {                                         \
        ensureDltContext();                      \
        DLT_LOG(GrubManager::m_ctx,              \
                DLT_LOG_INFO,                    \
                DLT_STRING(str));                \
    } while (0)

GrubManager::GrubManager(QObject *parent) : QObject(parent) {}

void GrubManager::runProcess(const QString &cmd, const QStringList &args) {
    GRB_LOG(QString("Run process: %1 %2").arg(cmd, args.join(" ")).toUtf8().constData());
    QProcess proc;
    proc.start(cmd, args);
    proc.waitForFinished(4000);
    updateStatus(QString::fromUtf8(proc.readAllStandardOutput()));
}

void GrubManager::updateStatus(const QString &output) {
    m_status = output.trimmed();
    parseGrubEnv(output);
    emit statusChanged();
}

void GrubManager::parseGrubEnv(const QString &output) {
    // Parse grubenv output
    QRegExp orderPattern("ORDER=([^\\s]+)");
    QRegExp defaultPattern("default=([^\\s]+)");
    QRegExp timeoutPattern("timeout=([^\\s]+)");
    QRegExp nextPattern("next_entry=([^\\s]+)");
    QRegExp savedPattern("saved_entry=([^\\s]+)");
    
    if (orderPattern.indexIn(output) != -1) {
        m_bootOrder = orderPattern.cap(1);
        // Parse slot order from ORDER value
        QStringList orders = m_bootOrder.split(" ");
        if (orders.size() >= 2) {
            m_slotAOrder = orders[0];
            m_slotBOrder = orders[1];
        }
    }
    
    if (defaultPattern.indexIn(output) != -1) {
        m_defaultEntry = defaultPattern.cap(1);
    }
    
    if (timeoutPattern.indexIn(output) != -1) {
        m_timeout = timeoutPattern.cap(1);
    }
    
    if (nextPattern.indexIn(output) != -1) {
        m_nextEntry = nextPattern.cap(1);
    }
    
    if (savedPattern.indexIn(output) != -1) {
        m_savedEntry = savedPattern.cap(1);
    }
    
    m_grubEnv = output;
}

void GrubManager::parseGrubVersion(const QString &output) {
    QRegExp versionPattern("GRUB\\s+version\\s+([^\\s]+)");
    if (versionPattern.indexIn(output) != -1) {
        m_grubVersion = versionPattern.cap(1);
    }
}

void GrubManager::refresh() {
    GRB_LOG("Refresh GRUB status requested");
    
    // Get grubenv information
    runProcess("/usr/bin/grub-editenv", {"/grubenv/grubenv", "list"});
    
    // Get GRUB version
    QProcess versionProc;
    versionProc.start("/usr/bin/grub-install", {"--version"});
    versionProc.waitForFinished(2000);
    parseGrubVersion(QString::fromUtf8(versionProc.readAllStandardOutput()));
}

void GrubManager::setBootOrder(const QString &order) {
    GRB_LOG(QString("Set boot order: %1").arg(order).toUtf8().constData());
    QProcess::execute("/usr/bin/grub-editenv", {"/grubenv/grubenv", "set", QString("ORDER=%1").arg(order)});
    refresh();
}

#undef GRB_LOG 