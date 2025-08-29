#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusConnectionInterface>
#include <dlt/dlt.h>

class UpdateAgentManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isUpdateActive READ isUpdateActive NOTIFY updateStatusChanged)
    Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateStatusChanged)
    Q_PROPERTY(int updateProgress READ updateProgress NOTIFY updateProgressChanged)
    Q_PROPERTY(bool isServiceRunning READ isServiceRunning NOTIFY serviceStatusChanged)

public:
    explicit UpdateAgentManager(QObject *parent = nullptr);
    ~UpdateAgentManager();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();
    Q_INVOKABLE void testProgressParsing(const QString& testLine);
    Q_INVOKABLE void testRealtimeMonitoring();

    // Property getters
    bool isUpdateActive() const { return m_isUpdateActive; }
    QString updateStatus() const { return m_updateStatus; }
    int updateProgress() const { return m_updateProgress; }
    bool isServiceRunning() const { return m_isServiceRunning; }

    // DLT context
    static DltContext m_ctx;

signals:
    void updateStatusChanged();
    void updateProgressChanged();
    void serviceStatusChanged();
    void updateStarted();
    void updateCompleted(bool success, const QString& message);

private slots:
    void onServiceStatusProcessFinished(int exitCode);
    void onRefreshTimer();
    void onDBusSignal(const QDBusMessage& message);

private:
    // Service monitoring
    bool m_isServiceRunning;
    QProcess* m_serviceStatusProcess;
    QTimer* m_refreshTimer;

    // Update monitoring
    bool m_isUpdateActive;
    QString m_updateStatus;
    int m_updateProgress;

    // Operation monitoring
    QString m_currentOperation;
    QTimer* m_operationPollTimer;

    // Rebooting progress timer
    QTimer* m_rebootProgressTimer;

    // D-Bus monitoring
    QDBusConnection m_dbusConnection;
    bool m_dbusConnected;

    // Status management
    void checkServiceStatus();
    void setupDBusMonitoring();
    void pollOperation();
    void updateStatusFromOperation(const QString& operation);
    void handleProgressSignal(int percentage);
    void handleCompletedSignal(bool success, const QString& message);
};
