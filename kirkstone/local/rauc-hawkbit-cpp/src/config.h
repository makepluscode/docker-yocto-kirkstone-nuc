#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Hawkbit Configuration
// These can be overridden by environment variables or config files in future versions

// Server configuration
const std::string HAWKBIT_SERVER_URL = "http://192.168.1.101:8080";
const std::string HAWKBIT_TENANT = "DEFAULT";
const std::string HAWKBIT_CONTROLLER_ID = "nuc-device-001";

// Timing configuration
const int POLL_INTERVAL_SECONDS = 1;
const int DOWNLOAD_TIMEOUT_SECONDS = 300;  // 5 minutes
const int INSTALLATION_TIMEOUT_SECONDS = 600;  // 10 minutes
const int HTTP_TIMEOUT_SECONDS = 30;

// File paths
const std::string BUNDLE_DOWNLOAD_PATH = "/tmp/hawkbit_update.raucb";
const std::string LOG_FILE_PATH = "/var/log/rauc-hawkbit-cpp.log";

// DLT Configuration
const std::string DLT_APP_NAME = "RHCP";
const std::string DLT_HAWK_CONTEXT = "HAWK";
const std::string DLT_RAUC_CONTEXT = "RAUC";
const std::string DLT_UPDATE_CONTEXT = "UPDT";

// Network configuration
const bool ENABLE_SSL_VERIFICATION = false;  // Set to true for production
const bool FOLLOW_REDIRECTS = true;

// Debug configuration
const bool ENABLE_VERBOSE_LOGGING = true;
const bool ENABLE_DEBUG_OUTPUT = true;

#endif // CONFIG_H 