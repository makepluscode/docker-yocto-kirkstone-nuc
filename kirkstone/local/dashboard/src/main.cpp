#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QDir>
#include "system_info.h"
#include "rauc_manager.h"
#include "rauc_system_manager.h"
#include "grub_manager.h"
#include <dlt/dlt.h>

static DltContext ctxUI;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // DLT initialization
    DLT_REGISTER_APP("DBO", "Dashboard Application");
    DLT_REGISTER_CONTEXT(ctxUI, "UIF", "UI Flow");

    app.setApplicationName("System Dashboard");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("NUC Systems");

    // Register QML types
    qmlRegisterType<SystemInfo>("SystemInfo", 1, 0, "SystemInfo");
    qmlRegisterType<RaucManager>("Rauc", 1, 0, "RaucManager");
    qmlRegisterType<RaucSystemManager>("RaucSystem", 1, 0, "RaucSystemManager");
    qmlRegisterType<GrubManager>("Grub", 1, 0, "GrubManager");

    QQmlApplicationEngine engine;
    
    // Load main QML file with fallback strategy
    bool loaded = false;
    
    // First try Qt resources
    qDebug() << "Trying to load QML from resources...";
    engine.load(QUrl(QStringLiteral("qrc:/DashboardMain.qml")));
    if (!engine.rootObjects().isEmpty()) {
        qDebug() << "Successfully loaded QML from resources";
        loaded = true;
    } else {
        qDebug() << "Failed to load from resources, trying file system...";
        // Fallback to file system
        QString qmlPath = "/usr/share/dashboard/qml/DashboardMain.qml";
        qDebug() << "Checking file:" << qmlPath;
        if (QFile::exists(qmlPath)) {
            qDebug() << "File exists, loading from file system...";
            engine.load(QUrl::fromLocalFile(qmlPath));
            if (!engine.rootObjects().isEmpty()) {
                qDebug() << "Successfully loaded QML from file system";
                loaded = true;
            } else {
                qDebug() << "Failed to load from file system";
            }
        } else {
            qDebug() << "File does not exist:" << qmlPath;
        }
    }
    
    if (!loaded) {
        qDebug() << "Failed to load QML from both resources and file system";
        return -1;
    }
    
    if (engine.rootObjects().isEmpty())
        return -1;

    int ret = app.exec();

    // Clean up DLT
    DLT_UNREGISTER_CONTEXT(ctxUI);
    DLT_UNREGISTER_APP();
    return ret;
} 