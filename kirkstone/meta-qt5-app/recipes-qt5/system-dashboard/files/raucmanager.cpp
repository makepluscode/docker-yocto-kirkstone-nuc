#include "raucmanager.h"
#include <QTextStream>
#include <QDebug>

RaucManager::RaucManager(QObject *parent) : QObject(parent) {
    refresh();
}

void RaucManager::runProcess(const QString &cmd, const QStringList &args) {
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
    runProcess("/usr/bin/rauc", {"status"});
}

static void setBootOrder(const QString &order) {
    QProcess::execute("/usr/bin/grub-editenv", {"/grubenv/grubenv", "set", QString("ORDER=%1").arg(order)});
}

void RaucManager::bootSlotA() {
    setBootOrder("A B");
    refresh();
}

void RaucManager::bootSlotB() {
    setBootOrder("B A");
    refresh();
} 