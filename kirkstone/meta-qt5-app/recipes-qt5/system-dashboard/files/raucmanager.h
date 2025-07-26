#pragma once
#include <QObject>
#include <QProcess>
#include <dlt/dlt.h>

class RaucManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString bootSlot READ bootSlot NOTIFY statusChanged)
    Q_PROPERTY(QString activatedSlot READ activatedSlot NOTIFY statusChanged)
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

signals:
    void statusChanged();

private:
    QString m_status;
    QString m_bootSlot;
    QString m_activatedSlot;

    void runProcess(const QString &cmd, const QStringList &args = {});
    void updateStatus(const QString &output);
    void parseStatus(const QString &output);
}; 