#pragma once
#include <QObject>
#include <QProcess>

class RaucManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
public:
    explicit RaucManager(QObject *parent = nullptr);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void bootSlotA();
    Q_INVOKABLE void bootSlotB();

    QString statusText() const { return m_status; }

signals:
    void statusChanged();

private:
    QString m_status;
    void runProcess(const QString &cmd, const QStringList &args = {});
    void updateStatus(const QString &output);
}; 