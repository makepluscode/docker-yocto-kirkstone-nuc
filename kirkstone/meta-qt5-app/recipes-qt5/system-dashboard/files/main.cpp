#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QDir>
#include "systeminfo.h"
#include "raucmanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    app.setApplicationName("System Dashboard");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("NUC Systems");

    // Register QML types
    qmlRegisterType<SystemInfo>("SystemInfo", 1, 0, "SystemInfo");
    qmlRegisterType<RaucManager>("Rauc", 1, 0, "RaucManager");

    QQmlApplicationEngine engine;
    
    // Load main QML file
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
} 