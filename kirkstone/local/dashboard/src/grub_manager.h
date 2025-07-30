#pragma once
#include <QObject>
#include <QProcess>
#include <QRegExp>
#include <dlt/dlt.h>

class GrubManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString bootOrder READ bootOrder NOTIFY statusChanged)
    Q_PROPERTY(QString defaultEntry READ defaultEntry NOTIFY statusChanged)
    Q_PROPERTY(QString timeout READ timeout NOTIFY statusChanged)
    Q_PROPERTY(QString nextEntry READ nextEntry NOTIFY statusChanged)
    Q_PROPERTY(QString savedEntry READ savedEntry NOTIFY statusChanged)
    Q_PROPERTY(QString grubVersion READ grubVersion NOTIFY statusChanged)
    Q_PROPERTY(QString grubEnv READ grubEnv NOTIFY statusChanged)
    Q_PROPERTY(QString slotAOrder READ slotAOrder NOTIFY statusChanged)
    Q_PROPERTY(QString slotBOrder READ slotBOrder NOTIFY statusChanged)
public:
    explicit GrubManager(QObject *parent = nullptr);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setBootOrder(const QString &order);

    // DLT context accessible from helper functions
    static DltContext m_ctx;
    QString statusText() const { return m_status; }
    QString bootOrder() const { return m_bootOrder; }
    QString defaultEntry() const { return m_defaultEntry; }
    QString timeout() const { return m_timeout; }
    QString nextEntry() const { return m_nextEntry; }
    QString savedEntry() const { return m_savedEntry; }
    QString grubVersion() const { return m_grubVersion; }
    QString grubEnv() const { return m_grubEnv; }
    QString slotAOrder() const { return m_slotAOrder; }
    QString slotBOrder() const { return m_slotBOrder; }

signals:
    void statusChanged();

private:
    QString m_status;
    QString m_bootOrder;
    QString m_defaultEntry;
    QString m_timeout;
    QString m_nextEntry;
    QString m_savedEntry;
    QString m_grubVersion;
    QString m_grubEnv;
    QString m_slotAOrder;
    QString m_slotBOrder;

    void runProcess(const QString &cmd, const QStringList &args = {});
    void updateStatus(const QString &output);
    void parseGrubEnv(const QString &output);
    void parseGrubVersion(const QString &output);
}; 