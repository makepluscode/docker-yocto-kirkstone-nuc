#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QFileSystemWatcher>
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
    Q_INVOKABLE void checkUpdateStatus();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();
    Q_INVOKABLE void testProgressParsing(const QString& testLine);
    Q_INVOKABLE void testStatusToggle();
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
    void onLogFileChanged(const QString& path);
    void onRefreshTimer();

private:
    // Service monitoring
    bool m_isServiceRunning;
    QProcess* m_serviceStatusProcess;
    QTimer* m_refreshTimer;
    
    // Update monitoring  
    bool m_isUpdateActive;
    QString m_updateStatus;
    int m_updateProgress;
    
    // Log monitoring
    QFileSystemWatcher* m_logWatcher;
    QString m_logFilePath;
    qint64 m_lastLogPosition;
    QProcess* m_journalFollowProcess;
    
    // Status parsing
    void parseLogContent();
    void parseLogLine(const QString& line);
    void checkServiceStatus();
    void setupLogMonitoring();
    void startRealtimeJournalMonitoring();
    void stopRealtimeJournalMonitoring();
    void updateProgressFromLog(const QString& line);
    void updateStatusFromLog(const QString& line);
};