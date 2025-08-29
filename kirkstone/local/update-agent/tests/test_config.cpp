#include <gtest/gtest.h>
#include "config.h"

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment if needed
    }
    
    void TearDown() override {
        // Clean up test environment if needed
    }
};

TEST_F(ConfigTest, ServerConfiguration) {
    // Test server configuration constants
    EXPECT_FALSE(HOST_SERVER_URL.empty());
    EXPECT_FALSE(HOST_TENANT.empty());
    EXPECT_FALSE(DEVICE_ID.empty());
    
    // Test URL format
    EXPECT_TRUE(HOST_SERVER_URL.find("http://") == 0 || HOST_SERVER_URL.find("https://") == 0);
    
    // Test tenant format (should not be empty)
    EXPECT_GT(HOST_TENANT.length(), 0);
    
    // Test device ID format (should not be empty)
    EXPECT_GT(DEVICE_ID.length(), 0);
}

TEST_F(ConfigTest, TimingConfiguration) {
    // Test timing configuration values
    EXPECT_GT(POLL_INTERVAL_SECONDS, 0);
    EXPECT_GT(DOWNLOAD_TIMEOUT_SECONDS, 0);
    EXPECT_GT(INSTALLATION_TIMEOUT_SECONDS, 0);
    EXPECT_GT(HTTP_TIMEOUT_SECONDS, 0);
    EXPECT_GT(PROGRESS_FEEDBACK_INTERVAL_SECONDS, 0);
    EXPECT_GT(INSTALLATION_TIMEOUT_SECONDS_MAIN, 0);
    EXPECT_GT(SERVICE_STATUS_CHECK_INTERVAL_SECONDS, 0);
    EXPECT_GT(MAIN_LOOP_SLEEP_SECONDS, 0);
    EXPECT_GT(REBOOT_DELAY_SECONDS, 0);
    EXPECT_GT(CLEANUP_DELAY_MS, 0);
    
    // Test reasonable timeout values
    EXPECT_LE(POLL_INTERVAL_SECONDS, 3600); // Max 1 hour
    EXPECT_LE(DOWNLOAD_TIMEOUT_SECONDS, 1800); // Max 30 minutes
    EXPECT_LE(INSTALLATION_TIMEOUT_SECONDS, 3600); // Max 1 hour
    EXPECT_LE(HTTP_TIMEOUT_SECONDS, 300); // Max 5 minutes
}

TEST_F(ConfigTest, FilePaths) {
    // Test file path configuration
    EXPECT_FALSE(UPDATE_BUNDLE_PATH.empty());
    EXPECT_FALSE(LOG_FILE_PATH.empty());
    EXPECT_FALSE(START_SIGNAL_FILE.empty());
    
    // Test path formats
    EXPECT_TRUE(UPDATE_BUNDLE_PATH.find("/tmp/") == 0);
    EXPECT_TRUE(LOG_FILE_PATH.find("/var/log/") == 0);
    EXPECT_TRUE(START_SIGNAL_FILE.find("/tmp/") == 0);
    
    // Test file extensions
    EXPECT_TRUE(UPDATE_BUNDLE_PATH.find(".raucb") != std::string::npos);
    EXPECT_TRUE(LOG_FILE_PATH.find(".log") != std::string::npos);
}

TEST_F(ConfigTest, LoggingConfiguration) {
    // Test logging configuration
    EXPECT_FALSE(LOG_APP_NAME.empty());
    EXPECT_FALSE(LOG_SERVER_CONTEXT.empty());
    EXPECT_FALSE(LOG_SYSTEM_CONTEXT.empty());
    EXPECT_FALSE(LOG_AGENT_CONTEXT.empty());
    
    // Test context name lengths (DLT context names are typically 4 characters)
    EXPECT_EQ(LOG_APP_NAME.length(), 4);
    EXPECT_EQ(LOG_SERVER_CONTEXT.length(), 4);
    EXPECT_EQ(LOG_SYSTEM_CONTEXT.length(), 4);
    EXPECT_EQ(LOG_AGENT_CONTEXT.length(), 4);
}

TEST_F(ConfigTest, NetworkConfiguration) {
    // Test network configuration
    // ENABLE_SSL_VERIFICATION and FOLLOW_REDIRECTS are boolean flags
    // Just verify they are defined (compilation test)
    EXPECT_TRUE(ENABLE_SSL_VERIFICATION == true || ENABLE_SSL_VERIFICATION == false);
    EXPECT_TRUE(FOLLOW_REDIRECTS == true || FOLLOW_REDIRECTS == false);
}