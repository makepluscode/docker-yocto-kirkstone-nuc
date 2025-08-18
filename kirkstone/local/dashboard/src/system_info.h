#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <QNetworkInterface>
#include <dlt/dlt.h>

class SystemInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
    Q_PROPERTY(QStringList cpuCoreUsage READ cpuCoreUsage NOTIFY cpuCoreUsageChanged)
    Q_PROPERTY(double memoryUsage READ memoryUsage NOTIFY memoryUsageChanged)
    Q_PROPERTY(qint64 totalMemory READ totalMemory NOTIFY totalMemoryChanged)
    Q_PROPERTY(qint64 usedMemory READ usedMemory NOTIFY usedMemoryChanged)
    Q_PROPERTY(qint64 freeMemory READ freeMemory NOTIFY freeMemoryChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(QString uptime READ uptime NOTIFY uptimeChanged)
    Q_PROPERTY(QString kernelVersion READ kernelVersion NOTIFY kernelVersionChanged)
    Q_PROPERTY(QString hostname READ hostname NOTIFY hostnameChanged)
    Q_PROPERTY(QString architecture READ architecture NOTIFY architectureChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(bool networkConnected READ networkConnected NOTIFY networkConnectedChanged)
    Q_PROPERTY(QString networkInterface READ networkInterface NOTIFY networkInterfaceChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(qint64 rootPartitionTotal READ rootPartitionTotal NOTIFY rootPartitionTotalChanged)
    Q_PROPERTY(qint64 rootPartitionUsed READ rootPartitionUsed NOTIFY rootPartitionUsedChanged)
    Q_PROPERTY(qint64 rootPartitionFree READ rootPartitionFree NOTIFY rootPartitionFreeChanged)
    Q_PROPERTY(double rootPartitionUsagePercent READ rootPartitionUsagePercent NOTIFY rootPartitionUsagePercentChanged)
    Q_PROPERTY(QString buildTime READ buildTime NOTIFY buildTimeChanged)
    Q_PROPERTY(QString yoctoVersion READ yoctoVersion NOTIFY yoctoVersionChanged)
    Q_PROPERTY(QString rootDevice READ rootDevice NOTIFY rootDeviceChanged)
    Q_PROPERTY(QString softwareVersion READ softwareVersion NOTIFY softwareVersionChanged)

public:
    explicit SystemInfo(QObject *parent = nullptr);

    // Getters
    double cpuUsage() const { return m_cpuUsage; }
    QStringList cpuCoreUsage() const { return m_cpuCoreUsage; }
    double memoryUsage() const { return m_memoryUsage; }
    qint64 totalMemory() const { return m_totalMemory; }
    qint64 usedMemory() const { return m_usedMemory; }
    qint64 freeMemory() const { return m_freeMemory; }
    double temperature() const { return m_temperature; }
    QString uptime() const { return m_uptime; }
    QString kernelVersion() const { return m_kernelVersion; }
    QString hostname() const { return m_hostname; }
    QString architecture() const { return m_architecture; }
    QString currentTime() const { return m_currentTime; }
    bool networkConnected() const { return m_networkConnected; }
    QString networkInterface() const { return m_networkInterface; }
    QString ipAddress() const { return m_ipAddress; }
    qint64 rootPartitionTotal() const { return m_rootPartitionTotal; }
    qint64 rootPartitionUsed() const { return m_rootPartitionUsed; }
    qint64 rootPartitionFree() const { return m_rootPartitionFree; }
    double rootPartitionUsagePercent() const { return m_rootPartitionUsagePercent; }
    QString buildTime() const { return m_buildTime; }
    QString yoctoVersion() const { return m_yoctoVersion; }
    QString rootDevice() const { return m_rootDevice; }
    QString softwareVersion() const { return m_softwareVersion; }

public slots:
    void updateSystemInfo();
    void updateTime();
    QString formatBytes(qint64 bytes);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void exitApplication();
    Q_INVOKABLE void rebootSystem();
    Q_INVOKABLE void startHawkbitUpdater();
    Q_INVOKABLE bool checkHawkbitServiceStatus();
    Q_INVOKABLE void stopHawkbitUpdater();
    Q_INVOKABLE QString getHawkbitServiceLogs(int lines = 20);
    Q_INVOKABLE void logUIEvent(const QString &event, const QString &details = "");
    Q_INVOKABLE QString getHawkbitServiceStatus();
    Q_INVOKABLE QString checkHawkbitConfiguration();
    Q_INVOKABLE bool testNetworkConnectivity();

signals:
    void cpuUsageChanged();
    void cpuCoreUsageChanged();
    void memoryUsageChanged();
    void totalMemoryChanged();
    void usedMemoryChanged();
    void freeMemoryChanged();
    void temperatureChanged();
    void uptimeChanged();
    void kernelVersionChanged();
    void hostnameChanged();
    void architectureChanged();
    void currentTimeChanged();
    void networkConnectedChanged();
    void networkInterfaceChanged();
    void ipAddressChanged();
    void rootPartitionTotalChanged();
    void rootPartitionUsedChanged();
    void rootPartitionFreeChanged();
    void rootPartitionUsagePercentChanged();
    void buildTimeChanged();
    void yoctoVersionChanged();
    void rootDeviceChanged();
    void softwareVersionChanged();
    void hawkbitServiceStatusChanged(bool active);
    void hawkbitUpdateDetected();
    void hawkbitUpdateFailed(const QString &error);

private:
    void updateCpuUsage();
    void updateCpuCoreUsage();
    void updateMemoryInfo();
    void updateTemperature();
    void updateUptime();
    void updateSystemDetails();
    void updateNetworkInfo();
    void updateDiskInfo();
    void updateBuildInfo();
    void updateRootDeviceInfo();
    void updateSoftwareVersion();
    QString readFileContent(const QString &filePath);

    // Member variables
    double m_cpuUsage;
    QStringList m_cpuCoreUsage;
    double m_memoryUsage;
    qint64 m_totalMemory;
    qint64 m_usedMemory;
    qint64 m_freeMemory;
    double m_temperature;
    QString m_uptime;
    QString m_kernelVersion;
    QString m_hostname;
    QString m_architecture;
    QString m_currentTime;
    bool m_networkConnected;
    QString m_networkInterface;
    QString m_ipAddress;
    qint64 m_rootPartitionTotal;
    qint64 m_rootPartitionUsed;
    qint64 m_rootPartitionFree;
    double m_rootPartitionUsagePercent;
    QString m_buildTime;
    QString m_yoctoVersion;
    QString m_rootDevice;
    QString m_softwareVersion;

    // Timers
    QTimer *m_updateTimer;
    QTimer *m_timeTimer;

    // CPU usage calculation
    qint64 m_lastCpuTotal;
    qint64 m_lastCpuIdle;

    // DLT context
    static DltContext m_dltCtx;
};

#endif // SYSTEM_INFO_H
