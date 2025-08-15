#ifndef UPDATERMANAGER_H
#define UPDATERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QLoggingCategory>
#include <QJsonObject>
#include <QJsonArray>

Q_DECLARE_LOGGING_CATEGORY(updaterManager)

class NetworkClient;
class DeploymentModel;
class BundleManager;
class ActivityLogModel;
class CertificateManager;

class UpdaterManager : public QObject
{
    Q_OBJECT
    
    // Server properties
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY serverStatusChanged)
    Q_PROPERTY(QString serverVersion READ serverVersion NOTIFY serverStatusChanged)
    Q_PROPERTY(bool useHttps READ useHttps WRITE setUseHttps NOTIFY httpsChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    
    // Statistics properties
    Q_PROPERTY(int deploymentCount READ deploymentCount NOTIFY deploymentsChanged)
    Q_PROPERTY(QString lastUpdate READ lastUpdate NOTIFY deploymentsChanged)
    Q_PROPERTY(int bundleCount READ bundleCount NOTIFY bundleStatsChanged)
    Q_PROPERTY(qreal bundleSize READ bundleSize NOTIFY bundleStatsChanged)
    
    // Models
    Q_PROPERTY(DeploymentModel* deploymentModel READ deploymentModel CONSTANT)
    Q_PROPERTY(ActivityLogModel* activityLogModel READ activityLogModel CONSTANT)
    Q_PROPERTY(BundleManager* bundleManager READ bundleManager CONSTANT)
    Q_PROPERTY(CertificateManager* certificateManager READ certificateManager CONSTANT)

public:
    explicit UpdaterManager(QObject *parent = nullptr);
    ~UpdaterManager();
    
    // Getters
    bool isConnected() const { return m_isConnected; }
    QString serverStatus() const { return m_serverStatus; }
    QString serverVersion() const { return m_serverVersion; }
    bool useHttps() const { return m_useHttps; }
    QString serverUrl() const { return m_serverUrl; }
    
    int deploymentCount() const { return m_deploymentCount; }
    QString lastUpdate() const { return m_lastUpdate; }
    int bundleCount() const { return m_bundleCount; }
    qreal bundleSize() const { return m_bundleSize; }
    
    DeploymentModel* deploymentModel() const { return m_deploymentModel; }
    ActivityLogModel* activityLogModel() const { return m_activityLogModel; }
    BundleManager* bundleManager() const { return m_bundleManager; }
    CertificateManager* certificateManager() const { return m_certificateManager; }
    
    // Setters
    void setUseHttps(bool useHttps);
    void setServerUrl(const QString &url);

public slots:
    // Initialization
    void initialize();
    void startPeriodicUpdates();
    void stopPeriodicUpdates();
    
    // Data operations
    void refreshData();
    void refreshServerStatus();
    void refreshDeployments();
    void refreshBundleStats();
    
    // Deployment operations
    QJsonArray getDeployments() const;
    bool createDeployment(const QJsonObject &deployment);
    bool deleteDeployment(const QString &deploymentId);
    bool activateDeployment(const QString &deploymentId);
    bool deactivateDeployment(const QString &deploymentId);
    
    // Bundle operations
    void downloadBundle(const QString &filename);
    void uploadBundle(const QString &filePath, const QString &version, const QString &description);
    
    // Network operations
    void testConnection();
    void reconnect();

signals:
    // Connection signals
    void connectionChanged();
    void serverStatusChanged();
    void httpsChanged();
    void serverUrlChanged();
    
    // Data signals
    void deploymentsChanged();
    void bundleStatsChanged();
    void activityLogChanged(const QString &message);
    
    // Error signals
    void errorOccurred(const QString &error);
    void warningOccurred(const QString &warning);
    void infoMessage(const QString &message);
    
    // Progress signals
    void uploadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onPeriodicUpdate();
    void onNetworkResponse(const QJsonObject &response);
    void onNetworkError(const QString &error);
    void onConnectionStateChanged(bool connected);

private:
    void updateConnectionState(bool connected);
    void logActivity(const QString &message);
    void handleServerResponse(const QJsonObject &response);
    void handleDeploymentsResponse(const QJsonArray &deployments);
    void updateStatistics();
    QString formatUrl(const QString &endpoint) const;
    
    // Network and models
    NetworkClient *m_networkClient;
    DeploymentModel *m_deploymentModel;
    BundleManager *m_bundleManager;
    ActivityLogModel *m_activityLogModel;
    CertificateManager *m_certificateManager;
    
    // Timers
    QTimer *m_periodicUpdateTimer;
    QTimer *m_reconnectTimer;
    
    // Server state
    bool m_isConnected;
    QString m_serverStatus;
    QString m_serverVersion;
    bool m_useHttps;
    QString m_serverUrl;
    
    // Statistics
    int m_deploymentCount;
    QString m_lastUpdate;
    int m_bundleCount;
    qreal m_bundleSize;
    
    // Configuration
    static constexpr int PERIODIC_UPDATE_INTERVAL = 30000; // 30 seconds
    static constexpr int RECONNECT_INTERVAL = 5000;        // 5 seconds
};

#endif // UPDATERMANAGER_H