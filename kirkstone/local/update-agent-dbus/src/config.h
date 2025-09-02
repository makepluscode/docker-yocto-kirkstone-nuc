#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Update Agent Configuration
// These can be overridden by environment variables or config files in future versions

// Host server configuration
const std::string HOST_SERVER_URL = "http://192.168.1.101:8080";
const std::string HOST_TENANT = "DEFAULT";
const std::string DEVICE_ID = "nuc-device-001";

// Timing configuration
const int POLL_INTERVAL_SECONDS = 10;
const int DOWNLOAD_TIMEOUT_SECONDS = 300;  // 5 minutes
const int INSTALLATION_TIMEOUT_SECONDS = 600;  // 10 minutes
const int HTTP_TIMEOUT_SECONDS = 30;
const int PROGRESS_FEEDBACK_INTERVAL_SECONDS = 3;  // Progress feedback cycle
const int INSTALLATION_TIMEOUT_SECONDS_MAIN = 300;  // Installation timeout in main loop (5 minutes)
const int SERVICE_STATUS_CHECK_INTERVAL_SECONDS = 10;  // Update service status check interval
const int MAIN_LOOP_SLEEP_SECONDS = 2;  // Main loop sleep interval
const int REBOOT_DELAY_SECONDS = 1;  // Delay before reboot (reduced from 2s to 1s)
const int CLEANUP_DELAY_MS = 100;  // Cleanup delay in milliseconds

// File paths
const std::string UPDATE_BUNDLE_PATH = "/tmp/update_bundle.raucb";
const std::string LOG_FILE_PATH = "/var/log/update-agent.log";
const std::string START_SIGNAL_FILE = "/tmp/update-agent-start-signal";

// Logging Configuration - Simplified
const std::string LOG_APP_NAME = "UAGT";
const std::string LOG_SERVER_CONTEXT = "SRVR";
const std::string LOG_SYSTEM_CONTEXT = "SYST";
const std::string LOG_AGENT_CONTEXT = "AGNT";

// Network configuration
const bool ENABLE_SSL_VERIFICATION = false;  // Set to true for production
const bool FOLLOW_REDIRECTS = true;

#endif // CONFIG_H
