#pragma once
#include <QObject>
#include <QProcess>
#include <QRegExp>
#include <dlt/dlt.h>

class RaucManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString bootSlot READ bootSlot NOTIFY statusChanged)
    Q_PROPERTY(QString activatedSlot READ activatedSlot NOTIFY statusChanged)
    Q_PROPERTY(QString compatible READ compatible NOTIFY statusChanged)
    Q_PROPERTY(QString variant READ variant NOTIFY statusChanged)
    Q_PROPERTY(QString booted READ booted NOTIFY statusChanged)
    Q_PROPERTY(QString bootPrimary READ bootPrimary NOTIFY statusChanged)
    Q_PROPERTY(QString slotAState READ slotAState NOTIFY statusChanged)
    Q_PROPERTY(QString slotAStatus READ slotAStatus NOTIFY statusChanged)
    Q_PROPERTY(QString slotBState READ slotBState NOTIFY statusChanged)
    Q_PROPERTY(QString slotBStatus READ slotBStatus NOTIFY statusChanged)
    Q_PROPERTY(QString slotADevice READ slotADevice NOTIFY statusChanged)
    Q_PROPERTY(QString slotBDevice READ slotBDevice NOTIFY statusChanged)
public:
    explicit RaucManager(QObject *parent = nullptr);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void bootSlotA();
    Q_INVOKABLE void bootSlotB();

    // DLT context accessible from helper functions
    static DltContext m_ctx;
    QString statusText() const { return m_status; }
    QString bootSlot() const { return m_bootSlot; }
    QString activatedSlot() const { return m_activatedSlot; }
    QString compatible() const { return m_compatible; }
    QString variant() const { return m_variant; }
    QString booted() const { return m_booted; }
    QString bootPrimary() const { return m_bootPrimary; }
    QString slotAState() const { return m_slotAState; }
    QString slotAStatus() const { return m_slotAStatus; }
    QString slotBState() const { return m_slotBState; }
    QString slotBStatus() const { return m_slotBStatus; }
    QString slotADevice() const { return m_slotADevice; }
    QString slotBDevice() const { return m_slotBDevice; }

signals:
    void statusChanged();

private:
    QString m_status;
    QString m_bootSlot;
    QString m_activatedSlot;
    QString m_compatible;
    QString m_variant;
    QString m_booted;
    QString m_bootPrimary;
    QString m_slotAState;
    QString m_slotAStatus;
    QString m_slotBState;
    QString m_slotBStatus;
    QString m_slotADevice;
    QString m_slotBDevice;

    void runProcess(const QString &cmd, const QStringList &args = {});
    void updateStatus(const QString &output);
    void parseStatus(const QString &output);
    void parseJsonStatus(const QString &jsonOutput);
};
