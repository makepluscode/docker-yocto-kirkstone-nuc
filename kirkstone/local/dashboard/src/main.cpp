#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QDir>
#include "system_info.h"
#include "rauc_manager.h"
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

    QQmlApplicationEngine engine;
    
    // Load main QML file with fallback strategy
    bool loaded = false;
    
    // First try Qt resources
    engine.load(QUrl(QStringLiteral("qrc:/dashboard_main.qml")));
    if (!engine.rootObjects().isEmpty()) {
        loaded = true;
    } else {
        // Fallback to file system
        QString qmlPath = "/usr/share/dashboard/qml/dashboard_main.qml";
        if (QFile::exists(qmlPath)) {
            engine.load(QUrl::fromLocalFile(qmlPath));
            if (!engine.rootObjects().isEmpty()) {
                loaded = true;
            }
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