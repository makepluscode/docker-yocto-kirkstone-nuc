#ifndef RAUC_SYSTEM_MANAGER_H
#define RAUC_SYSTEM_MANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QTimer>
#include <dlt/dlt.h>

class RaucSystemManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentBootSlot READ currentBootSlot NOTIFY currentBootSlotChanged)
    Q_PROPERTY(QString bootOrder READ bootOrder NOTIFY bootOrderChanged)
    Q_PROPERTY(QString slotAStatus READ slotAStatus NOTIFY slotAStatusChanged)
    Q_PROPERTY(QString slotBStatus READ slotBStatus NOTIFY slotBStatusChanged)
    Q_PROPERTY(bool slotAHealthy READ slotAHealthy NOTIFY slotAHealthyChanged)
    Q_PROPERTY(bool slotBHealthy READ slotBHealthy NOTIFY slotBHealthyChanged)
    Q_PROPERTY(bool updateInProgress READ updateInProgress NOTIFY updateInProgressChanged)
    Q_PROPERTY(bool bundleExists READ bundleExists NOTIFY bundleExistsChanged)
    Q_PROPERTY(QString bundlePath READ bundlePath NOTIFY bundlePathChanged)
    Q_PROPERTY(qint64 bundleSize READ bundleSize NOTIFY bundleSizeChanged)
    Q_PROPERTY(QString bundleSizeFormatted READ bundleSizeFormatted NOTIFY bundleSizeChanged)
    Q_PROPERTY(QString bundleModified READ bundleModified NOTIFY bundleModifiedChanged)

public:
    explicit RaucSystemManager(QObject *parent = nullptr);
    
    // Getters
    QString currentBootSlot() const { return m_currentBootSlot; }
    QString bootOrder() const { return m_bootOrder; }
    QString slotAStatus() const { return m_slotAStatus; }
    QString slotBStatus() const { return m_slotBStatus; }
    bool slotAHealthy() const { return m_slotAStatus == "good"; }
    bool slotBHealthy() const { return m_slotBStatus == "good"; }
    bool updateInProgress() const { return m_updateInProgress; }
    bool bundleExists() const { return m_bundleExists; }
    QString bundlePath() const { return m_bundlePath; }
    qint64 bundleSize() const { return m_bundleSize; }
    QString bundleSizeFormatted() const { return m_bundleSizeFormatted; }
    QString bundleModified() const { return m_bundleModified; }

public slots:
    // RAUC operations
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void bootToSlotA();
    Q_INVOKABLE void bootToSlotB();
    Q_INVOKABLE bool checkRaucBundle();
    Q_INVOKABLE void installRaucBundle();
    Q_INVOKABLE void startSoftwareUpdate();
    
    // System operations
    Q_INVOKABLE void rebootSystem();

signals:
    void currentBootSlotChanged();
    void bootOrderChanged();
    void slotAStatusChanged();
    void slotBStatusChanged();
    void slotAHealthyChanged();
    void slotBHealthyChanged();
    void updateInProgressChanged();
    void updateCompleted(bool success);
    void updateProgress(int percentage, const QString &message);
    void bundleExistsChanged();
    void bundlePathChanged();
    void bundleSizeChanged();
    void bundleModifiedChanged();

private slots:
    void onRaucProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRaucProcessError(QProcess::ProcessError error);

private:
    void updateCurrentBootSlot();
    void updateBootOrder();
    void updateSlotStatus();
    void updateBundleInfo();
    void executeGrubScript(const QString &script);
    QString executeRaucCommand(const QStringList &arguments);
    void setUpdateInProgress(bool inProgress);
    QString formatBytes(qint64 bytes);
    
    // Member variables
    QString m_currentBootSlot;
    QString m_bootOrder;
    QString m_slotAStatus;
    QString m_slotBStatus;
    bool m_updateInProgress;
    
    // Bundle information
    bool m_bundleExists;
    QString m_bundlePath;
    qint64 m_bundleSize;
    QString m_bundleSizeFormatted;
    QString m_bundleModified;
    
    // Process management
    QProcess *m_raucProcess;
    QTimer *m_statusTimer;
    
    // Constants
    static const QString RAUC_BUNDLE_PATH;
    static const QString GRUB_CONFIG_PATH;
    
    // DLT context accessible from helper functions
    static DltContext m_ctx;
};

#endif // RAUC_SYSTEM_MANAGER_H