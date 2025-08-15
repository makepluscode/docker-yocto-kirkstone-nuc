#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QDir>
#include "system_info.h"
#include "rauc_manager.h"
#include "rauc_system_manager.h"
#include "grub_manager.h"
#include "update_agent_manager.h"
#include <dlt/dlt.h>

static DltContext ctxUI;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // DLT initialization
    DLT_REGISTER_APP("DBO", "Dashboard Application");
    DLT_REGISTER_CONTEXT(ctxUI, "UIF", "UI Flow");
    
    DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Dashboard application starting"));
    DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Qt version: "), DLT_STRING(QT_VERSION_STR));

    app.setApplicationName("System Dashboard");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("NUC Systems");

    // Register QML types
    DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Registering QML types"));
    qmlRegisterType<SystemInfo>("SystemInfo", 1, 0, "SystemInfo");
    qmlRegisterType<RaucManager>("Rauc", 1, 0, "RaucManager");
    qmlRegisterType<RaucSystemManager>("RaucSystem", 1, 0, "RaucSystemManager");
    qmlRegisterType<GrubManager>("Grub", 1, 0, "GrubManager");
    qmlRegisterType<UpdateAgentManager>("UpdateAgent", 1, 0, "UpdateAgentManager");

    QQmlApplicationEngine engine;
    
    // Load main QML file with fallback strategy
    bool loaded = false;
    
    // First try Qt resources
    DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Attempting to load QML from Qt resources"));
    qDebug() << "Trying to load QML from resources...";
    engine.load(QUrl(QStringLiteral("qrc:/DashboardMain.qml")));
    if (!engine.rootObjects().isEmpty()) {
        DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("QML loaded successfully from Qt resources"));
        qDebug() << "Successfully loaded QML from resources";
        loaded = true;
    } else {
        DLT_LOG(ctxUI, DLT_LOG_WARN, DLT_STRING("Failed to load QML from resources, trying filesystem"));
        qDebug() << "Failed to load from resources, trying file system...";
        // Fallback to file system
        QString qmlPath = "/usr/share/dashboard/qml/DashboardMain.qml";
        DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Checking QML file: "), DLT_STRING(qmlPath.toUtf8().constData()));
        qDebug() << "Checking file:" << qmlPath;
        if (QFile::exists(qmlPath)) {
            DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("QML file exists, loading from filesystem"));
            qDebug() << "File exists, loading from file system...";
            engine.load(QUrl::fromLocalFile(qmlPath));
            if (!engine.rootObjects().isEmpty()) {
                DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("QML loaded successfully from filesystem"));
                qDebug() << "Successfully loaded QML from file system";
                loaded = true;
            } else {
                DLT_LOG(ctxUI, DLT_LOG_ERROR, DLT_STRING("Failed to load QML from filesystem"));
                qDebug() << "Failed to load from file system";
            }
        } else {
            DLT_LOG(ctxUI, DLT_LOG_ERROR, DLT_STRING("QML file not found: "), DLT_STRING(qmlPath.toUtf8().constData()));
            qDebug() << "File does not exist:" << qmlPath;
        }
    }
    
    if (!loaded) {
        DLT_LOG(ctxUI, DLT_LOG_FATAL, DLT_STRING("Critical: Failed to load QML from all sources"));
        qDebug() << "Failed to load QML from both resources and file system";
        return -1;
    }
    
    if (engine.rootObjects().isEmpty()) {
        DLT_LOG(ctxUI, DLT_LOG_FATAL, DLT_STRING("Critical: No QML root objects found"));
        return -1;
    }

    DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Dashboard UI loaded successfully, entering main event loop"));
    int ret = app.exec();

    DLT_LOG(ctxUI, DLT_LOG_INFO, DLT_STRING("Dashboard application exiting"));
    // Clean up DLT
    DLT_UNREGISTER_CONTEXT(ctxUI);
    DLT_UNREGISTER_APP();
    return ret;
} 