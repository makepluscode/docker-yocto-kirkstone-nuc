# SW Update GUI Requirements

## Document Information
- **Document ID**: 03-GUI_Requirements  
- **Version**: 1.0
- **Date**: 2025-08-31
- **Author**: SW Update Team

## 1. Overview

This document defines the GUI requirements for the SW Update System, consisting of two main components: (1) Host Updater Tool and (2) Target System GUI. The system utilizes DDS (Data Distribution Service) for real-time communication between components to ensure reliable and efficient update operations.

### 1.1 System Architecture
- **Host Updater Tool**: Qt6/QML-based management application running on host PC
- **Target System GUI**: Qt5/QML-based dashboard application running on target device
- **Communication Protocol**: DDS for real-time data distribution and status synchronization
- **Update Protocol**: RAUC-based A/B boot system for secure updates

## 2. Host Updater Tool Requirements

### 2.1 Functional Requirements

#### 2.1.1 Main Window and Layout
- **SW-REQ-GUI-001**: The system SHALL provide a fixed-size main window (1280x720 pixels)
- **SW-REQ-GUI-002**: The system SHALL implement a top-bottom layout design with bundle information panel on top and logs panel on bottom
- **SW-REQ-GUI-003**: The system SHALL prevent window resizing to maintain consistent layout
- **SW-REQ-GUI-004**: The system SHALL display application title "Updater - OTA Update Management" in window header

#### 2.1.2 Bundle Management
- **SW-REQ-GUI-005**: The system SHALL automatically scan bundle directory every 5 seconds for .raucb files
- **SW-REQ-GUI-006**: The system SHALL identify and display the latest bundle automatically based on modification time
- **SW-REQ-GUI-007**: The system SHALL display bundle information including filename, size, and creation timestamp
- **SW-REQ-GUI-008**: The system SHALL support manual bundle directory management through file system operations
- **SW-REQ-GUI-009**: The system SHALL validate bundle file extensions (.raucb) during scanning

#### 2.1.3 Server Management
- **SW-REQ-GUI-010**: The system SHALL provide start/stop server controls with visual indicators
- **SW-REQ-GUI-011**: The system SHALL display real-time server status (Running/Stopped) with color-coded indicators
- **SW-REQ-GUI-012**: The system SHALL show server port and endpoint information when running
- **SW-REQ-GUI-013**: The system SHALL handle server startup failures gracefully with error messages

#### 2.1.4 Real-time Logging
- **SW-REQ-GUI-014**: The system SHALL display real-time log messages with timestamp formatting
- **SW-REQ-GUI-015**: The system SHALL implement auto-scroll functionality for log viewer
- **SW-REQ-GUI-016**: The system SHALL maintain log history during session
- **SW-REQ-GUI-017**: The system SHALL categorize log messages by severity (info, warning, error)

### 2.2 DDS Communication Requirements

#### 2.2.1 DDS Publisher Interface
- **SW-REQ-GUI-020**: The system SHALL publish deployment commands via DDS topic "UpdateCommands"
- **SW-REQ-GUI-021**: The system SHALL publish bundle availability notifications via DDS topic "BundleAvailability"  
- **SW-REQ-GUI-022**: The system SHALL publish server status updates via DDS topic "ServerStatus"
- **SW-REQ-GUI-023**: The system SHALL handle DDS publisher connection failures with automatic retry mechanism

#### 2.2.2 DDS Subscriber Interface
- **SW-REQ-GUI-024**: The system SHALL subscribe to target device status via DDS topic "DeviceStatus"
- **SW-REQ-GUI-025**: The system SHALL subscribe to update progress via DDS topic "UpdateProgress"
- **SW-REQ-GUI-026**: The system SHALL subscribe to installation feedback via DDS topic "InstallationFeedback"
- **SW-REQ-GUI-027**: The system SHALL handle DDS subscriber connection failures with reconnection logic

### 2.3 HTTP Server Requirements

#### 2.3.1 Bundle Distribution
- **SW-REQ-GUI-030**: The system SHALL serve bundle files via HTTP GET /download/{filename} endpoint
- **SW-REQ-GUI-031**: The system SHALL support HTTPS with TLS encryption for secure downloads
- **SW-REQ-GUI-032**: The system SHALL implement file size validation before serving bundles
- **SW-REQ-GUI-033**: The system SHALL log all bundle download requests with client information

#### 2.3.2 API Endpoints
- **SW-REQ-GUI-034**: The system SHALL provide deployment management API at /admin/deployments
- **SW-REQ-GUI-035**: The system SHALL support poll endpoint at /{tenant}/controller/v1/{device-id}
- **SW-REQ-GUI-036**: The system SHALL return appropriate HTTP status codes (200, 204, 404, 500)
- **SW-REQ-GUI-037**: The system SHALL implement health check endpoint at /health

## 3. Target System GUI Requirements

### 3.1 Functional Requirements

#### 3.1.1 Dashboard Layout
- **SW-REQ-GUI-040**: The system SHALL display a grid-based dashboard with multiple information cards
- **SW-REQ-GUI-041**: The system SHALL organize cards in a 3x2 layout for optimal information presentation
- **SW-REQ-GUI-042**: The system SHALL support full-screen display mode for industrial environments
- **SW-REQ-GUI-043**: The system SHALL implement hardware-accelerated graphics using Qt5 EGLFS

#### 3.1.2 System Monitoring Cards
- **SW-REQ-GUI-044**: The system SHALL display CPU information card with real-time usage percentage
- **SW-REQ-GUI-045**: The system SHALL display memory information card with RAM usage statistics
- **SW-REQ-GUI-046**: The system SHALL display network information card with interface status and IP addresses
- **SW-REQ-GUI-047**: The system SHALL display storage information card with disk usage and partition status

#### 3.1.3 Update Status Display
- **SW-REQ-GUI-048**: The system SHALL display boot information card showing current and inactive slots
- **SW-REQ-GUI-049**: The system SHALL show RAUC system status including compatible string and boot slot
- **SW-REQ-GUI-050**: The system SHALL display update progress during installation operations
- **SW-REQ-GUI-051**: The system SHALL show last update timestamp and version information

### 3.2 DDS Communication Requirements

#### 3.2.1 DDS Publisher Interface
- **SW-REQ-GUI-055**: The system SHALL publish device status via DDS topic "DeviceStatus" every 5 seconds
- **SW-REQ-GUI-056**: The system SHALL publish system metrics via DDS topic "SystemMetrics" every 10 seconds
- **SW-REQ-GUI-057**: The system SHALL publish installation feedback via DDS topic "InstallationFeedback"
- **SW-REQ-GUI-058**: The system SHALL publish boot slot status via DDS topic "BootSlotStatus"

#### 3.2.2 DDS Subscriber Interface
- **SW-REQ-GUI-059**: The system SHALL subscribe to update commands via DDS topic "UpdateCommands"
- **SW-REQ-GUI-060**: The system SHALL subscribe to bundle availability via DDS topic "BundleAvailability"
- **SW-REQ-GUI-061**: The system SHALL subscribe to server status via DDS topic "ServerStatus"
- **SW-REQ-GUI-062**: The system SHALL handle DDS communication failures with visual status indicators

### 3.3 RAUC Integration Requirements

#### 3.3.1 Update Management
- **SW-REQ-GUI-065**: The system SHALL interface with RAUC via D-Bus for update operations
- **SW-REQ-GUI-066**: The system SHALL monitor RAUC installation progress and display real-time status
- **SW-REQ-GUI-067**: The system SHALL handle RAUC installation completion signals (success/failure)
- **SW-REQ-GUI-068**: The system SHALL support RAUC mark operations for boot slot management

#### 3.3.2 System Information
- **SW-REQ-GUI-069**: The system SHALL display RAUC system compatible string and variant
- **SW-REQ-GUI-070**: The system SHALL show current boot slot and primary boot configuration
- **SW-REQ-GUI-071**: The system SHALL display slot status information (good, bad, active)
- **SW-REQ-GUI-072**: The system SHALL show bundle information including version and checksum

## 4. Technical Requirements

### 4.1 DDS Configuration
- **SW-REQ-GUI-075**: The system SHALL use RTI Connext DDS or Eclipse Cyclone DDS implementation
- **SW-REQ-GUI-076**: The system SHALL configure DDS domain ID as configurable parameter
- **SW-REQ-GUI-077**: The system SHALL implement DDS QoS policies for reliable communication
- **SW-REQ-GUI-078**: The system SHALL handle DDS participant discovery within 5 seconds

### 4.2 Performance Requirements
- **SW-REQ-GUI-080**: The system SHALL update GUI elements within 100ms of receiving DDS messages
- **SW-REQ-GUI-081**: The system SHALL maintain responsive UI during update operations
- **SW-REQ-GUI-082**: The system SHALL limit memory usage to maximum 512MB for host tool
- **SW-REQ-GUI-083**: The system SHALL limit memory usage to maximum 256MB for target GUI

### 4.3 Reliability Requirements
- **SW-REQ-GUI-085**: The system SHALL recover automatically from DDS communication failures
- **SW-REQ-GUI-086**: The system SHALL maintain operation when network connectivity is temporarily lost
- **SW-REQ-GUI-087**: The system SHALL preserve update state during unexpected shutdowns
- **SW-REQ-GUI-088**: The system SHALL validate bundle integrity before installation

### 4.4 Security Requirements
- **SW-REQ-GUI-090**: The system SHALL verify bundle signatures using RAUC certificate validation
- **SW-REQ-GUI-091**: The system SHALL use encrypted communication channels for bundle downloads
- **SW-REQ-GUI-092**: The system SHALL implement DDS security plugins for authenticated communication
- **SW-REQ-GUI-093**: The system SHALL log all security-related events for audit purposes

## 5. User Interface Requirements

### 5.1 Visual Design Standards
- **SW-REQ-GUI-095**: The system SHALL use consistent color scheme across all UI components
- **SW-REQ-GUI-096**: The system SHALL provide high contrast display modes for industrial environments
- **SW-REQ-GUI-097**: The system SHALL support scalable fonts for different display resolutions
- **SW-REQ-GUI-098**: The system SHALL implement touch-friendly controls for tablet interfaces

### 5.2 Accessibility Requirements
- **SW-REQ-GUI-100**: The system SHALL provide keyboard navigation support for all interactive elements
- **SW-REQ-GUI-101**: The system SHALL implement screen reader compatibility for accessibility
- **SW-REQ-GUI-102**: The system SHALL support customizable font sizes for visual impairment
- **SW-REQ-GUI-103**: The system SHALL provide audio feedback for critical update operations

### 5.3 Internationalization
- **SW-REQ-GUI-105**: The system SHALL support English and Korean language localization
- **SW-REQ-GUI-106**: The system SHALL implement dynamic language switching without restart
- **SW-REQ-GUI-107**: The system SHALL format dates and times according to system locale
- **SW-REQ-GUI-108**: The system SHALL support Unicode text rendering for international characters

## 6. Integration Requirements

### 6.1 System Integration
- **SW-REQ-GUI-110**: The system SHALL integrate with systemd for service management
- **SW-REQ-GUI-111**: The system SHALL support DLT (Diagnostic Log and Trace) for system logging
- **SW-REQ-GUI-112**: The system SHALL interface with GRUB bootloader for boot slot management
- **SW-REQ-GUI-113**: The system SHALL support udev events for hardware change detection

### 6.2 Development Tools Integration
- **SW-REQ-GUI-115**: The system SHALL support Qt Creator IDE for GUI development
- **SW-REQ-GUI-116**: The system SHALL provide CMake build system configuration
- **SW-REQ-GUI-117**: The system SHALL support cross-compilation for target hardware
- **SW-REQ-GUI-118**: The system SHALL include unit tests for critical GUI components

## 7. Deployment Requirements

### 7.1 Host Tool Deployment
- **SW-REQ-GUI-120**: The system SHALL provide standalone executable for Windows and Linux hosts
- **SW-REQ-GUI-121**: The system SHALL include all required Qt6 libraries in distribution package
- **SW-REQ-GUI-122**: The system SHALL support installer package creation for easy deployment
- **SW-REQ-GUI-123**: The system SHALL provide configuration file for DDS settings

### 7.2 Target System Deployment
- **SW-REQ-GUI-125**: The system SHALL integrate with Yocto build system for target image creation
- **SW-REQ-GUI-126**: The system SHALL provide systemd service files for automatic startup
- **SW-REQ-GUI-127**: The system SHALL support EGLFS platform for framebuffer graphics
- **SW-REQ-GUI-128**: The system SHALL include DDS runtime libraries in target image

## 8. Validation and Testing

### 8.1 Functional Testing
- **SW-REQ-GUI-130**: The system SHALL pass bundle detection and management test cases
- **SW-REQ-GUI-131**: The system SHALL pass DDS communication reliability test cases
- **SW-REQ-GUI-132**: The system SHALL pass RAUC integration test cases for update operations
- **SW-REQ-GUI-133**: The system SHALL pass UI responsiveness test cases under load conditions

### 8.2 Integration Testing
- **SW-REQ-GUI-135**: The system SHALL pass end-to-end update workflow test cases
- **SW-REQ-GUI-136**: The system SHALL pass multi-device deployment test cases
- **SW-REQ-GUI-137**: The system SHALL pass failover and recovery test cases
- **SW-REQ-GUI-138**: The system SHALL pass security validation test cases

### 8.3 Performance Testing
- **SW-REQ-GUI-140**: The system SHALL pass memory usage validation under normal operations
- **SW-REQ-GUI-141**: The system SHALL pass CPU usage validation during update operations
- **SW-REQ-GUI-142**: The system SHALL pass network bandwidth usage validation
- **SW-REQ-GUI-143**: The system SHALL pass concurrent user handling validation for host tool

## 9. Maintenance and Support

### 9.1 Logging and Diagnostics
- **SW-REQ-GUI-145**: The system SHALL provide comprehensive logging for troubleshooting
- **SW-REQ-GUI-146**: The system SHALL support log level configuration (DEBUG, INFO, WARN, ERROR)
- **SW-REQ-GUI-147**: The system SHALL implement log rotation to prevent disk space issues
- **SW-REQ-GUI-148**: The system SHALL provide diagnostic information collection utilities

### 9.2 Configuration Management
- **SW-REQ-GUI-150**: The system SHALL support configuration file validation
- **SW-REQ-GUI-151**: The system SHALL provide default configuration fallback mechanisms
- **SW-REQ-GUI-152**: The system SHALL support runtime configuration updates where applicable
- **SW-REQ-GUI-153**: The system SHALL document all configuration parameters and their effects

---

**End of Document**

This requirements document defines the comprehensive GUI specifications for both Host Updater Tool and Target System components, with DDS-based communication architecture ensuring reliable and real-time update management capabilities.