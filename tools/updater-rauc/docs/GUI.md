# ARCRO Updater - GUI Documentation

ARCRO (Advanced RAUC Control & Rollout Operations) provides a modern Qt6 QML-based graphical user interface for deploying RAUC bundles to embedded Linux devices.

## Overview

The GUI is designed with a Raspberry Pi Imager-inspired interface, featuring a clean, intuitive workflow for RAUC bundle deployment. The interface supports both Korean and English languages.

## Design Principles

### UI Layout
- **Raspberry Pi Imager Style**: Clean, card-based design with visual selection elements
- **Three-Step Workflow**: Computer → Package → Location selection
- **Progress Tracking**: Visual progress bar with real-time status updates
- **Modal Success Dialog**: Completion notification with return to main screen

### Color Scheme
- **Primary Color**: `#c51a4a` (Raspberry Pi Red)
- **Secondary Color**: `#8cc04b` (Success Green)
- **Background**: `#f5f5f5` (Light Gray)
- **Card Color**: `#ffffff` (White)
- **Text Color**: `#333333` (Dark Gray)
- **Border Color**: `#e0e0e0` (Light Border)

## User Interface Components

### Main Window
- **Title**: "ARCRO Updater"
- **Dimensions**: 960x480 pixels (Raspberry Pi Imager horizontal layout)
- **Layout**: Horizontal split with left cards and right button area
- **Margins**: 30px with 15px top margin for header
- **Header Font**: 22px bold title text

### Selection Page

#### Horizontal Layout
- **Main Split**: Left side (cards) + Right side (button)
- **Left Container**: Maximum 600px width, fills height
- **Card Spacing**: 12px between cards
- **Card Height**: 65px per selector (compact for horizontal layout)
- **Card Style**: White background, 2px border, 8px radius

#### 1. Computer Selector (Fixed)
```
🖥️ 컴퓨터
   GUI IPC            ✓
```
- **Status**: Always enabled and selected
- **Purpose**: Indicates GUI-based Inter-Process Communication  
- **Visual**: 36px red circular icon with computer emoji
- **Text**: 15px title, 11px subtitle
- **Border**: 2px light gray border

#### 2. Update Package Selector (Interactive)
```
📦 업데이트 패키지
   RAUC 번들을 선택하세요    ⚠️/✓
```
- **Function**: Opens file dialog to select RAUC bundle (.raucb)
- **States**:
  - **Unselected**: Gray 36px icon with warning symbol, gray border
  - **Selected**: Green 36px icon with checkmark, green border
- **File Types**: Filters for `*.raucb` files
- **Text**: 15px title, 11px subtitle with filename display

#### 3. Location Selector (Fixed)
```
💾 위치
   /dev/sdX           ✓
```
- **Status**: Always enabled and selected
- **Purpose**: Indicates target device location
- **Visual**: 36px red circular icon with disk emoji
- **Text**: 15px title, 11px subtitle
- **Border**: 2px light gray border

#### Next Button (Right Side)
- **Position**: Right side panel, centered vertically
- **Label**: "다음" (Next)
- **Size**: 160px width, 45px height
- **State**: Enabled only when package is selected
- **Style**: 22px rounded rectangle with primary color
- **Font**: 16px bold white text
- **Panel Width**: 280px preferred, 220px minimum
- **Action**: Transitions to progress page and starts update

### Progress Page

#### Layout (Vertical Center)
- **Container Width**: Maximum 600px or 80% of parent width
- **Spacing**: 25px between elements
- **Centered**: All elements center-aligned vertically

#### Progress Information
- **Title**: "업데이트 중..." (Updating...) - 20px bold
- **Status Text**: Dynamic status messages - 13px normal
- **Layout**: Centered text elements with 8px spacing

#### Progress Bar
- **Size**: Full width, 18px height (compact for horizontal layout)
- **Style**: 9px rounded rectangle with animated fill
- **Colors**: Light gray background (#e0e0e0) with green progress fill
- **Text**: 11px bold percentage display (0-100%) centered
- **Animation**: Smooth width transitions (300ms duration)

#### Cancel Button
- **Label**: "취소" (Cancel)
- **Size**: 120px width, 40px height (compact)
- **Style**: 20px rounded red button
- **Font**: 13px bold white text
- **Function**: Cancels ongoing update and returns to selection page

### Success Dialog

#### Modal Popup
- **Title**: "업데이트 완료" (Update Complete)
- **Size**: 400x250 pixels
- **Position**: Centered over main window
- **Background**: White card with 8px radius

#### Content
- **Icon**: ✅ (64px large success checkmark)
- **Message**: "업데이트가 성공적으로 완료되었습니다!" (16px text)
- **Button**: "확인" (OK) - Green button returns to selection page
- **Layout**: Centered column with 20px spacing

## Technical Implementation

### QML Structure
```
ApplicationWindow
├── Header (Title Text)
├── StackLayout (Page Container)
│   ├── Selection Page
│   │   ├── Computer Card
│   │   ├── Package Card (Interactive)
│   │   ├── Location Card
│   │   └── Next Button
│   └── Progress Page
│       ├── Progress Info
│       ├── Progress Bar
│       └── Cancel Button
├── FileDialog (Package Selection)
└── Success Dialog (Modal)
```

### State Management
- **updateInProgress**: Boolean flag controlling page visibility
- **selectedPackage**: String holding selected bundle file path
- **progress**: Integer (0-100) for progress bar display

### Controller Integration
The GUI connects to the Python backend through the `RaucController` class:

#### Properties
- `bundlePath`: Selected RAUC bundle file path
- `progress`: Current update progress percentage
- `isUpdating`: Update operation status

#### Signals
- `statusChanged(status)`: Updates progress status text
- `progressChanged(value)`: Updates progress bar value
- `updateCompleted(success, message)`: Handles update completion

#### Slots
- `startUpdate()`: Initiates the update process
- `cancelUpdate()`: Cancels ongoing update operation

## File Dialog Configuration

### Settings
- **Title**: "RAUC 번들 선택" (Select RAUC Bundle)
- **File Filters**: 
  - "RAUC bundles (*.raucb)"
  - "All files (*)"
- **Selection**: Single file selection mode

### Path Handling
- Automatically removes "file://" prefix from selected paths
- Updates package selector with filename display
- Enables Next button when valid file is selected

## User Workflow

### Standard Operation Flow
1. **Launch Application**: ARCRO Updater opens with selection page
2. **Select Package**: Click package card to open file dialog
3. **Choose Bundle**: Select `.raucb` file from filesystem
4. **Verify Selection**: Package card shows selected filename with checkmark
5. **Start Update**: Click "다음" button to begin process
6. **Monitor Progress**: Watch progress bar and status messages
7. **Handle Completion**: 
   - **Success**: Modal dialog appears with confirmation
   - **Cancel**: Click cancel button to abort operation
8. **Return to Start**: Click "확인" or complete cancellation to reset

### Error Handling
- **No Package Selected**: Next button remains disabled
- **Update Failure**: Returns to selection page (error handling to be enhanced)
- **Connection Issues**: Status messages indicate connection problems

## Internationalization

### Language Support
- **Primary Language**: Korean (한국어)
- **UI Text**: All user-facing text uses `qsTr()` for translation support
- **Future Enhancement**: English translation files can be added

### Text Elements
- Window titles, button labels, status messages
- File dialog prompts and error messages
- Success and confirmation dialogs

## Accessibility Features

### Visual Design
- **High Contrast**: Clear color separation between elements
- **Large Touch Targets**: Buttons and cards sized for easy interaction
- **Clear Typography**: Readable fonts with appropriate sizing
- **Status Indicators**: Visual checkmarks and warning symbols

### Keyboard Navigation
- Standard Qt keyboard navigation support
- Tab order follows logical UI flow
- Enter/Space key activation for buttons

## Development Notes

### File Structure
```
src/rauc_updater/gui/
├── main.py              # Application entry point
├── controller.py        # Python backend controller
└── qml/
    ├── main.qml        # Main application window
    ├── main-old.qml    # Previous multi-card design (backup)
    └── [other components as needed]
```

### Dependencies
- **PyQt6**: Core GUI framework
- **QtQuick 2.15**: QML runtime
- **QtQuick.Controls 2.15**: UI controls
- **QtQuick.Layouts 1.15**: Layout management
- **QtQuick.Dialogs**: File selection dialog

### Build Requirements
- Python 3.9+
- PyQt6 6.5.0+
- Qt6 QML runtime components

## Performance Considerations

### Threaded Operations
- Update operations run in separate `QThread` (UpdateWorker)
- UI remains responsive during long-running operations
- Progress updates via Qt signal/slot mechanism

### Resource Management
- Worker threads properly cleaned up after completion
- File handles closed after bundle selection
- Memory efficient QML property bindings

## Future Enhancements

### Planned Features
- **Multi-device Support**: Manage multiple target devices
- **Configuration Persistence**: Remember recent settings
- **Enhanced Error Handling**: Detailed error messages and recovery
- **Logging Integration**: Real-time log viewer component
- **Theme Support**: Dark/light mode toggle

### Technical Improvements
- **Validation**: Bundle file integrity checking
- **Network Status**: Connection quality indicators
- **Resume Capability**: Interrupted update recovery
- **Batch Operations**: Multiple bundle deployment

## Troubleshooting

### Common Issues
1. **GUI Not Loading**: Check PyQt6 installation and QML dependencies
2. **File Dialog Empty**: Verify file system permissions
3. **Progress Not Updating**: Check controller signal connections
4. **Update Fails**: Verify network connectivity and target device status

### Debug Mode
Enable debug output by setting environment variable:
```bash
QT_LOGGING_RULES="qml.debug=true" arcro-gui
```

### Log Files
Application logs available through Python backend controller integration.

## Update Process Workflow

### Three-Step Update Scenario

The ARCRO updater follows a comprehensive 3-step process to ensure safe and reliable RAUC bundle deployment:

#### Step 1: Setup Phase (0-25% Progress)
**Objective**: Establish connection and verify system readiness

**Operations**:
- **Connection Establishment**: Connect to target device via SSH using multiple authentication methods
  - Try SSH key authentication (id_rsa, id_ecdsa, id_ed25519)
  - Fallback to password authentication (root, empty password)
  - Display connection status with retry mechanism
- **RAUC Availability Check**: Verify RAUC is installed and operational on target
  - Execute `which rauc` to confirm RAUC binary exists
  - Execute `rauc --version` to verify RAUC functionality
  - Display RAUC version information (e.g., "RAUC 1.13")
- **System Status Verification**: Check current RAUC system state
  - Execute `rauc status` to get current boot configuration
  - Display active slot information (A or B)
  - Verify slot states and boot status
  - Check compatible system identifier

**Progress Tracking**:
- 0%: Initial connection attempt
- 10%: SSH connection established
- 15%: RAUC availability confirmed
- 20%: System status retrieved
- 25%: Setup phase completed

**GUI Indicators**:
- Status: "Connecting to target..."
- Progress bar: 0-25%
- Connection status updates in real-time

#### Step 2: Software Update Phase (25-85% Progress)
**Objective**: Transfer and install RAUC bundle

**Sub-Phase 2A: Bundle Upload (25-50%)**
- **Bundle Validation**: Verify selected .raucb file integrity
- **Disk Space Check**: Ensure sufficient space on target device
  - Execute `df -h /tmp` to check available storage
  - Verify bundle size vs. available space
- **Secure Transfer**: Upload bundle via SFTP with progress tracking
  - Transfer to `/tmp/[bundle-filename].raucb`
  - Real-time upload progress with speed indication
  - File integrity verification after upload
- **Upload Verification**: Confirm successful transfer
  - Execute `ls -lh /tmp/[bundle].raucb` to verify file size
  - Compare local and remote file sizes

**Sub-Phase 2B: RAUC Installation (50-85%)**
- **Pre-Installation Check**: Verify bundle file exists on target
  - Execute `test -f /tmp/[bundle].raucb`
- **Installation Execution**: Install RAUC bundle with progress monitoring
  - Execute `rauc install /tmp/[bundle].raucb`
  - Parse RAUC installation output for progress percentage
  - Display installation status messages ("Installing...", "Writing image...")
- **Installation Verification**: Confirm successful completion
  - Monitor for "Installing done" message
  - Verify "succeeded" status in installation output

**Progress Tracking**:
- 25-50%: Bundle upload with real-time transfer progress
- 50-85%: Installation progress from RAUC output parsing
- Status messages: "Uploading bundle...", "Installing bundle...", "Installing: Processing..."

**GUI Indicators**:
- Upload progress bar with MB/s speed indication
- Installation progress with RAUC-specific status messages
- File transfer completion confirmation

#### Step 3: Validation Phase (85-100% Progress)
**Objective**: Reboot system and verify successful update

**Sub-Phase 3A: System Reboot (85-90%)**
- **Reboot Initiation**: Trigger system restart to activate new slot
  - Execute `nohup sh -c 'sleep 3 && reboot' > /dev/null 2>&1 & exit`
  - Graceful SSH connection closure
  - 3-second delay to ensure command execution
- **Offline Detection**: Monitor device going offline
  - Detect SSH connection loss (expected behavior)
  - Wait 10 seconds for complete shutdown
- **Connection Loss Handling**: Treat disconnection as normal reboot process
  - Catch SSH exceptions during reboot
  - Interpret connection timeouts as successful reboot initiation

**Sub-Phase 3B: Reconnection and Verification (90-100%)**
- **Reconnection Attempts**: Wait for device to come back online
  - Retry SSH connections every 5 seconds
  - Maximum wait time: 120 seconds (configurable)
  - Use same authentication methods as initial connection
- **Boot Slot Verification**: Confirm successful slot transition
  - Execute `rauc status` on reconnected device
  - Parse "Booted from:" information to verify slot change
  - Example transitions:
    - Before: `Booted from: rootfs.1 (B)`
    - After: `Booted from: rootfs.0 (A)`
- **Update Confirmation**: Validate complete update success
  - Display new boot slot information
  - Confirm RAUC system status is healthy
  - Show "Device rebooted successfully" message

**Progress Tracking**:
- 85%: Reboot initiated
- 85-90%: Waiting for device offline/online cycle
- 90-95%: Reconnection in progress
- 95-100%: RAUC status verification
- 100%: Update validation completed

**GUI Indicators**:
- Status: "Rebooting device...", "Waiting for reboot...", "Device reconnected - checking RAUC status..."
- Success dialog: "업데이트가 성공적으로 완료되었습니다! 장치가 새 이미지로 재부팅되었습니다."

### Error Handling and Recovery

**Connection Failures**:
- Multiple authentication method attempts
- Connection timeout handling
- Network connectivity verification
- User guidance for manual intervention

**Upload Failures**:
- Insufficient disk space warnings
- Network interruption recovery
- File corruption detection
- Retry mechanisms for failed transfers

**Installation Failures**:
- RAUC compatibility checking
- Bundle signature verification
- Installation error parsing and display
- Rollback capability preservation

**Reboot/Validation Failures**:
- Reboot timeout handling (120s default)
- Failed reconnection scenarios
- RAUC status verification failures
- Manual verification instructions for users

### Successful Completion Indicators

**Visual Confirmations**:
- ✅ Green checkmarks for each completed phase
- 📊 Progress bar reaching 100%
- 🎉 Success dialog with reboot confirmation
- 🔄 Boot slot transition display (A ↔ B)

**System State Changes**:
- Active boot slot switched (A to B or B to A)
- New image successfully loaded and operational
- RAUC system status showing "good" boot status
- Device accessible via SSH with new firmware

**User Feedback**:
- Real-time progress updates throughout all phases
- Detailed status messages for each operation
- Clear success/failure indications
- Automatic return to initial state for next update