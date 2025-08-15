import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../style"

ToolBar {
    id: root
    
    property var updaterManager
    
    function updateStatus() {
        connectionIndicator.connected = updaterManager.isConnected
        connectionText.text = updaterManager.isConnected ? strings.connected : strings.disconnected
    }
    
    background: Rectangle {
        color: style.colors.secondary
        
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: style.colors.border
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: style.spacing.large
        anchors.rightMargin: style.spacing.large
        spacing: style.spacing.medium
        
        // App branding
        RowLayout {
            spacing: style.spacing.small
            
            Text {
                text: style.icons.server
                font.pixelSize: style.dimensions.iconSizeLarge
                color: style.colors.textLight
            }
            
            Text {
                text: strings.appName
                font.family: style.fonts.family
                font.pixelSize: style.fonts.title
                font.weight: Font.Bold
                color: style.colors.textLight
            }
            
            Rectangle {
                width: 1
                height: 24
                color: style.colors.border
                visible: versionText.text.length > 0
            }
            
            Text {
                id: versionText
                text: strings.appVersion
                font.family: style.fonts.family
                font.pixelSize: style.fonts.small
                color: style.colors.textMuted
            }
        }
        
        Item { Layout.fillWidth: true }
        
        // Status indicators
        RowLayout {
            spacing: style.spacing.medium
            
            // Connection status
            RowLayout {
                spacing: style.spacing.small
                
                Rectangle {
                    id: connectionIndicator
                    width: style.spacing.medium
                    height: style.spacing.medium
                    radius: width / 2
                    color: connected ? style.colors.statusConnected : style.colors.statusDisconnected
                    
                    property bool connected: false
                    
                    SequentialAnimation on opacity {
                        running: !connectionIndicator.connected
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: style.animations.normal }
                        NumberAnimation { to: 1.0; duration: style.animations.normal }
                    }
                }
                
                Text {
                    id: connectionText
                    text: strings.disconnected
                    font.family: style.fonts.family
                    font.pixelSize: style.fonts.small
                    color: style.colors.textLight
                }
            }
            
            Rectangle {
                width: 1
                height: 24
                color: style.colors.border
            }
            
            // Server status
            RowLayout {
                spacing: style.spacing.small
                
                Text {
                    text: style.icons.activity
                    font.pixelSize: style.dimensions.iconSize
                    color: style.colors.textLight
                }
                
                Text {
                    text: updaterManager ? updaterManager.serverStatus : strings.stopped
                    font.family: style.fonts.family
                    font.pixelSize: style.fonts.small
                    color: style.colors.textLight
                }
            }
            
            Rectangle {
                width: 1
                height: 24
                color: style.colors.border
            }
            
            // Actions
            RowLayout {
                spacing: style.spacing.small
                
                Button {
                    text: style.icons.refresh
                    font.pixelSize: style.dimensions.iconSize
                    flat: true
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                        radius: style.dimensions.buttonRadius
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: parent.font.pixelSize
                        color: style.colors.textLight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    ToolTip.text: strings.refresh
                    ToolTip.visible: hovered
                    
                    onClicked: {
                        if (updaterManager) {
                            updaterManager.refreshData()
                        }
                    }
                }
                
                Button {
                    text: style.icons.settings
                    font.pixelSize: style.dimensions.iconSize
                    flat: true
                    
                    background: Rectangle {
                        color: parent.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                        radius: style.dimensions.buttonRadius
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: parent.font.pixelSize
                        color: style.colors.textLight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    ToolTip.text: strings.settings
                    ToolTip.visible: hovered
                    
                    onClicked: {
                        // Settings action will be handled by parent
                    }
                }
            }
        }
    }
}