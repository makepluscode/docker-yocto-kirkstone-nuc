# C++ Code Separation Guide

## Overview
The dashboard C++ code has been successfully separated into focused, single-responsibility components for better maintainability and organization.

## Before: Monolithic Structure
Previously, all functionality was combined in `SystemInfo` class:
```cpp
// system_info.h/.cpp contained:
- System monitoring (CPU, memory, network, temperature)
- RAUC boot management
- Software update functionality
- Boot slot switching
- Application control
```

## After: Separated Architecture

### 1. SystemInfo Class (src/system_info.h/.cpp)
**Purpose**: Pure system monitoring and information collection
**Responsibilities**:
- CPU usage and core monitoring
- Memory usage tracking
- Network interface status
- Temperature monitoring
- System uptime and details
- Disk usage information
- Build and version information

**Properties**:
```cpp
Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
Q_PROPERTY(QStringList cpuCoreUsage READ cpuCoreUsage NOTIFY cpuCoreUsageChanged)
Q_PROPERTY(double memoryUsage READ memoryUsage NOTIFY memoryUsageChanged)
Q_PROPERTY(qint64 totalMemory READ totalMemory NOTIFY totalMemoryChanged)
Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
Q_PROPERTY(QString uptime READ uptime NOTIFY uptimeChanged)
Q_PROPERTY(QString hostname READ hostname NOTIFY hostnameChanged)
Q_PROPERTY(bool networkConnected READ networkConnected NOTIFY networkConnectedChanged)
// ... and more system properties
```

**Public Methods**:
```cpp
Q_INVOKABLE void refresh();           // Refresh all system information
Q_INVOKABLE void exitApplication();   // Exit the application
QString formatBytes(qint64 bytes);    // Utility for formatting byte values
```

### 2. RaucSystemManager Class (src/rauc_system_manager.h/.cpp)
**Purpose**: RAUC software update and boot management
**Responsibilities**:
- Boot slot management (A/B slots)
- RAUC bundle installation
- Software update process
- Boot order configuration
- System rebooting for updates

**Properties**:
```cpp
Q_PROPERTY(QString currentBootSlot READ currentBootSlot NOTIFY currentBootSlotChanged)
Q_PROPERTY(QString bootOrder READ bootOrder NOTIFY bootOrderChanged)
Q_PROPERTY(QString slotAStatus READ slotAStatus NOTIFY slotAStatusChanged)
Q_PROPERTY(QString slotBStatus READ slotBStatus NOTIFY slotBStatusChanged)
Q_PROPERTY(bool slotAHealthy READ slotAHealthy NOTIFY slotAHealthyChanged)
Q_PROPERTY(bool slotBHealthy READ slotBHealthy NOTIFY slotBHealthyChanged)
Q_PROPERTY(bool updateInProgress READ updateInProgress NOTIFY updateInProgressChanged)
```

**Public Methods**:
```cpp
Q_INVOKABLE void refreshStatus();        // Refresh RAUC status
Q_INVOKABLE void bootToSlotA();         // Switch to boot slot A
Q_INVOKABLE void bootToSlotB();         // Switch to boot slot B
Q_INVOKABLE bool checkRaucBundle();     // Check for update bundle
Q_INVOKABLE void installRaucBundle();   // Install RAUC bundle
Q_INVOKABLE void startSoftwareUpdate(); // Start complete update process
Q_INVOKABLE void rebootSystem();        // Reboot the system
```

**Signals**:
```cpp
void updateCompleted(bool success);
void updateProgress(int percentage, const QString &message);
```

## QML Integration Changes

### Before:
```qml
// All functionality through SystemInfo
SystemInfo {
    id: systemInfo
}

// Boot info accessed via SystemInfo methods
systemInfo.getCurrentBootSlot()
systemInfo.getBootOrder()
systemInfo.getSlotAStatus()
```

### After:
```qml
// Separate managers for different responsibilities
SystemInfo {
    id: systemInfo
}

RaucSystemManager {
    id: raucSystemManager
}

// Boot info accessed via dedicated manager
raucSystemManager.currentBootSlot
raucSystemManager.bootOrder
raucSystemManager.slotAStatus
```

## File Structure

### New Files Added:
- `src/rauc_system_manager.h` - RAUC manager header
- `src/rauc_system_manager.cpp` - RAUC manager implementation

### Modified Files:
- `src/system_info.h` - Removed RAUC-related methods
- `src/system_info.cpp` - Removed RAUC-related implementations
- `src/main.cpp` - Added RaucSystemManager registration
- `qml/DashboardMain.qml` - Added RaucSystemManager instance
- `qml/DashboardCard30.qml` - Updated to use RaucSystemManager
- `CMakeLists.txt` - Added new source files

## Benefits of Separation

### 1. **Single Responsibility Principle**
- Each class has one clear purpose
- Easier to understand and maintain
- Reduced coupling between components

### 2. **Better Testing**
- System monitoring can be tested independently
- RAUC functionality can be mocked/tested separately
- Clear interfaces for unit testing

### 3. **Enhanced Maintainability**
- Changes to RAUC logic don't affect system monitoring
- Easier to add new features to specific areas
- Clear code organization

### 4. **Improved Performance**
- System monitoring timer (2s) separate from RAUC status timer (5s)
- No unnecessary updates when only one subsystem changes
- More efficient resource usage

### 5. **Parallel Development**
- Different developers can work on system monitoring vs RAUC features
- Clear API boundaries
- Reduced merge conflicts

## Usage Examples

### System Monitoring:
```qml
DashboardCard01 {
    title: "CPU Load"

    Column {
        anchors.centerIn: parent

        Text {
            text: systemInfo.cpuUsage.toFixed(1) + "%"
            color: systemInfo.cpuUsage > 80 ? "#ff4444" : "#44ff44"
        }

        // CPU core usage display
        Repeater {
            model: systemInfo.cpuCoreUsage
            Text { text: "Core " + (index+1) + ": " + modelData + "%" }
        }
    }
}
```

### RAUC Management:
```qml
DashboardCard30 {
    title: "Boot Info"

    Column {
        anchors.centerIn: parent

        CardInfoRow {
            label: "Current Slot"
            value: raucSystemManager.currentBootSlot
        }

        CardInfoRow {
            label: "Boot Order"
            value: raucSystemManager.bootOrder
        }

        Button {
            text: "Switch to Slot A"
            enabled: !raucSystemManager.updateInProgress
            onClicked: raucSystemManager.bootToSlotA()
        }
    }
}
```

## Build Status
✅ **Successfully builds** with no errors
✅ **All functionality preserved**
✅ **Clean separation** achieved
✅ **QML integration** updated
⚠️ Only deprecation warnings (non-breaking)

## Migration Complete
The code separation is now complete and ready for production use. The dashboard maintains all existing functionality while providing a much cleaner, more maintainable architecture.
