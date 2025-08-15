pragma Singleton
import QtQuick 6.0

QtObject {
    id: style
    
    // Colors
    readonly property QtObject colors: QtObject {
        // Primary colors
        readonly property color primary: "#3498db"
        readonly property color secondary: "#2c3e50"
        readonly property color success: "#27ae60"
        readonly property color danger: "#e74c3c"
        readonly property color warning: "#f39c12"
        readonly property color info: "#17a2b8"
        
        // Background colors
        readonly property color background: "#ecf0f1"
        readonly property color surface: "#ffffff"
        readonly property color cardBackground: "#ffffff"
        
        // Text colors
        readonly property color textPrimary: "#2c3e50"
        readonly property color textSecondary: "#7f8c8d"
        readonly property color textMuted: "#95a5a6"
        readonly property color textLight: "#ffffff"
        
        // Border colors
        readonly property color border: "#bdc3c7"
        readonly property color borderLight: "#ecf0f1"
        readonly property color borderDark: "#95a5a6"
        
        // Status colors
        readonly property color statusActive: "#27ae60"
        readonly property color statusInactive: "#e74c3c"
        readonly property color statusPending: "#f39c12"
        readonly property color statusConnected: "#27ae60"
        readonly property color statusDisconnected: "#e74c3c"
    }
    
    // Typography
    readonly property QtObject fonts: QtObject {
        readonly property int extraSmall: 10
        readonly property int small: 12
        readonly property int medium: 14
        readonly property int large: 16
        readonly property int extraLarge: 18
        readonly property int title: 20
        readonly property int heading: 24
        
        readonly property string family: "Inter, -apple-system, BlinkMacSystemFont, sans-serif"
        readonly property string monospace: "JetBrains Mono, Consolas, Monaco, monospace"
    }
    
    // Spacing
    readonly property QtObject spacing: QtObject {
        readonly property int extraSmall: 4
        readonly property int small: 8
        readonly property int medium: 12
        readonly property int large: 16
        readonly property int extraLarge: 24
        readonly property int huge: 32
    }
    
    // Dimensions
    readonly property QtObject dimensions: QtObject {
        readonly property int buttonHeight: 36
        readonly property int inputHeight: 40
        readonly property int cardRadius: 8
        readonly property int buttonRadius: 6
        readonly property int borderWidth: 1
        readonly property int iconSize: 16
        readonly property int iconSizeLarge: 24
        readonly property int iconSizeHuge: 32
        
        // Card dimensions
        readonly property int cardMinWidth: 200
        readonly property int cardMinHeight: 120
        readonly property int cardPadding: 16
        
        // Header dimensions
        readonly property int headerHeight: 60
        readonly property int toolbarHeight: 48
    }
    
    // Animations
    readonly property QtObject animations: QtObject {
        readonly property int fast: 150
        readonly property int normal: 250
        readonly property int slow: 400
        readonly property string easing: "OutCubic"
    }
    
    // Shadows
    readonly property QtObject shadows: QtObject {
        readonly property color light: Qt.rgba(0, 0, 0, 0.1)
        readonly property color medium: Qt.rgba(0, 0, 0, 0.15)
        readonly property color dark: Qt.rgba(0, 0, 0, 0.25)
        
        readonly property int lightOffset: 2
        readonly property int mediumOffset: 4
        readonly property int darkOffset: 8
        
        readonly property int lightRadius: 4
        readonly property int mediumRadius: 8
        readonly property int darkRadius: 16
    }
    
    // Icons (using Unicode emojis for simplicity)
    readonly property QtObject icons: QtObject {
        readonly property string server: "🖥️"
        readonly property string deployment: "📦"
        readonly property string bundle: "💾"
        readonly property string iconUpload: "📤"
        readonly property string iconDownload: "📥"
        readonly property string iconDelete: "🗑️"
        readonly property string iconEdit: "✏️"
        readonly property string iconRefresh: "🔄"
        readonly property string iconSettings: "⚙️"
        readonly property string iconInfo: "ℹ️"
        readonly property string iconWarning: "⚠️"
        readonly property string iconError: "❌"
        readonly property string iconSuccess: "✅"
        readonly property string iconActivity: "📊"
        readonly property string iconLog: "📝"
        readonly property string iconConnection: "🔗"
        readonly property string iconSecurity: "🔒"
        readonly property string iconPlus: "➕"
        readonly property string iconMinus: "➖"
        readonly property string iconClose: "✖️"
        readonly property string iconCheck: "✓"
        readonly property string iconArrow: "→"
        readonly property string iconMenu: "☰"
    }
}