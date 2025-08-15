#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Update Agent Configuration
// These can be overridden by environment variables or config files in future versions

// Update server configuration
const std::string UPDATE_SERVER_URL = "http://192.168.1.101:8080";
const std::string UPDATE_TENANT = "DEFAULT";
const std::string DEVICE_ID = "nuc-device-001";

// Timing configuration
const int POLL_INTERVAL_SECONDS = 10;
const int DOWNLOAD_TIMEOUT_SECONDS = 300;  // 5 minutes
const int INSTALLATION_TIMEOUT_SECONDS = 600;  // 10 minutes
const int HTTP_TIMEOUT_SECONDS = 30;

// File paths
const std::string UPDATE_BUNDLE_PATH = "/tmp/update_bundle.raucb";
const std::string LOG_FILE_PATH = "/var/log/update-agent.log";
const std::string START_SIGNAL_FILE = "/tmp/update-agent-start-signal";

// Logging Configuration - Simplified
const std::string LOG_APP_NAME = "UPDT";
const std::string LOG_SERVER_CONTEXT = "SRVR";
const std::string LOG_SYSTEM_CONTEXT = "SYST";
const std::string LOG_AGENT_CONTEXT = "AGNT";

// Network configuration
const bool ENABLE_SSL_VERIFICATION = false;  // Set to true for production
const bool FOLLOW_REDIRECTS = true;

#endif // CONFIG_H
