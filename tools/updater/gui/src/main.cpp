#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(gui, "updater.gui")

int main(int argc, char *argv[])
{
    // Set application properties
    QGuiApplication::setApplicationName("Updater Server GUI");
    QGuiApplication::setApplicationVersion("0.2.0");
    QGuiApplication::setOrganizationName("Updater Project");
    QGuiApplication::setOrganizationDomain("updater.local");
    
    QGuiApplication app(argc, argv);
    
    qCInfo(gui) << "Starting Updater Server GUI" << QGuiApplication::applicationVersion();
    
    // Create QML engine
    QQmlApplicationEngine engine;
    
    // Try to load QML file from multiple possible locations
    QStringList qmlPaths = {
        "qrc:/Updater/qml/main.qml",
        "qrc:/qml/main.qml", 
        "qml/main.qml",
        "../qml/main.qml"
    };
    
    bool loaded = false;
    for (const QString &path : qmlPaths) {
        QUrl url(path);
        if (path.startsWith("qrc:")) {
            url = QUrl(path);
        } else {
            url = QUrl::fromLocalFile(path);
        }
        
        engine.load(url);
        if (!engine.rootObjects().isEmpty()) {
            qCInfo(gui) << "Successfully loaded QML from:" << path;
            loaded = true;
            break;
        }
        engine.clearComponentCache();
    }
    
    if (!loaded) {
        qCCritical(gui) << "Failed to load any QML file, creating minimal window";
        
        // Create a minimal QML content as fallback
        engine.loadData(QByteArray(R"(
            import QtQuick 6.0
            import QtQuick.Controls 6.0
            import QtQuick.Window 6.0
            
            ApplicationWindow {
                width: 800
                height: 600
                visible: true
                title: "Updater Server GUI"
                
                Rectangle {
                    anchors.fill: parent
                    color: "#f0f0f0"
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 20
                        
                        Text {
                            text: "🔄 Updater Server GUI"
                            font.pixelSize: 24
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "Version 0.2.0"
                            font.pixelSize: 16
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: "GUI application is running successfully!"
                            font.pixelSize: 14
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Button {
                            text: "Test Button"
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                console.log("Button clicked!")
                            }
                        }
                    }
                }
            }
        )"));
        
        if (engine.rootObjects().isEmpty()) {
            qCCritical(gui) << "Failed to create even minimal window, exiting";
            return -1;
        }
    }
    
    qCInfo(gui) << "GUI loaded successfully";
    
    // Start event loop
    int result = app.exec();
    
    qCInfo(gui) << "Application exiting with code:" << result;
    return result;
}