#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <dlt/dlt.h>
#include "hawkbit_client.h"
#include "rauc_client.h"
#include "config.h"

DLT_DECLARE_CONTEXT(hawkbitContext);
DLT_DECLARE_CONTEXT(raucContext);
DLT_DECLARE_CONTEXT(updateContext);

volatile bool running = true;

// Global variables for update tracking
std::string current_execution_id;
bool update_in_progress = false;

void signalHandler(int signal) {
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Received signal: "), DLT_INT(signal));
    running = false;
}

void onRaucProgress(int progress) {
    DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("RAUC installation progress: "), DLT_INT(progress), DLT_STRING("%"));
    
    // Send progress feedback to Hawkbit if we have an execution ID
    if (!current_execution_id.empty()) {
        // This would need to be called from the main loop or a separate thread
        // For now, we'll just log it
        DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Progress feedback needed for execution: "), 
                DLT_STRING(current_execution_id.c_str()), DLT_STRING(" Progress: "), DLT_INT(progress));
    }
}

void onRaucCompleted(bool success, const std::string& message) {
    if (success) {
        DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("RAUC installation completed successfully: "), DLT_STRING(message.c_str()));
    } else {
        DLT_LOG(raucContext, DLT_LOG_ERROR, DLT_STRING("RAUC installation failed: "), DLT_STRING(message.c_str()));
    }
    
    // Mark update as completed
    update_in_progress = false;
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update process completed. Success: "), DLT_BOOL(success));
}

bool performUpdate(HawkbitClient& hawkbitClient, RaucClient& raucClient, const UpdateInfo& update_info) {
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Starting update process"));
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Version: "), DLT_STRING(update_info.version.c_str()));
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Description: "), DLT_STRING(update_info.description.c_str()));
    
    current_execution_id = update_info.execution_id;
    update_in_progress = true;
    
    // Step 1: Send started feedback
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Sending started feedback to Hawkbit"));
    if (!hawkbitClient.sendStartedFeedback(update_info.execution_id)) {
        DLT_LOG(updateContext, DLT_LOG_ERROR, DLT_STRING("Failed to send started feedback"));
        update_in_progress = false;
        return false;
    }
    
    // Step 2: Download bundle
    std::string bundle_path = BUNDLE_DOWNLOAD_PATH;
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Downloading bundle from: "), DLT_STRING(update_info.download_url.c_str()));
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Bundle will be saved to: "), DLT_STRING(bundle_path.c_str()));
    
    if (!hawkbitClient.downloadBundle(update_info.download_url, bundle_path)) {
        DLT_LOG(updateContext, DLT_LOG_ERROR, DLT_STRING("Failed to download bundle"));
        hawkbitClient.sendFinishedFeedback(update_info.execution_id, false, "Download failed");
        update_in_progress = false;
        return false;
    }
    
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Bundle downloaded successfully"));
    
    // Step 3: Send download progress feedback
    hawkbitClient.sendProgressFeedback(update_info.execution_id, 50, "Bundle downloaded successfully");
    
    // Step 4: Install bundle with RAUC
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Starting RAUC bundle installation"));
    if (!raucClient.installBundle(bundle_path)) {
        DLT_LOG(updateContext, DLT_LOG_ERROR, DLT_STRING("Failed to start bundle installation"));
        hawkbitClient.sendFinishedFeedback(update_info.execution_id, false, "Failed to start installation");
        update_in_progress = false;
        return false;
    }
    
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Bundle installation started successfully"));
    
    // Step 5: Monitor installation progress
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Monitoring installation progress"));
    std::string status;
    int timeout_counter = 0;
    const int MAX_TIMEOUT = INSTALLATION_TIMEOUT_SECONDS;
    
    while (update_in_progress && timeout_counter < MAX_TIMEOUT) {
        if (raucClient.getStatus(status)) {
            DLT_LOG(updateContext, DLT_LOG_DEBUG, DLT_STRING("Current RAUC status: "), DLT_STRING(status.c_str()));
            
            if (status == "idle") {
                DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Installation completed successfully"));
                hawkbitClient.sendFinishedFeedback(update_info.execution_id, true, "Installation completed successfully");
                break;
            } else if (status == "failed") {
                DLT_LOG(updateContext, DLT_LOG_ERROR, DLT_STRING("Installation failed"));
                hawkbitClient.sendFinishedFeedback(update_info.execution_id, false, "Installation failed");
                break;
            }
        } else {
            DLT_LOG(updateContext, DLT_LOG_WARN, DLT_STRING("Failed to get RAUC status"));
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        timeout_counter++;
    }
    
    if (timeout_counter >= MAX_TIMEOUT) {
        DLT_LOG(updateContext, DLT_LOG_ERROR, DLT_STRING("Installation timeout after "), DLT_INT(MAX_TIMEOUT), DLT_STRING(" seconds"));
        hawkbitClient.sendFinishedFeedback(update_info.execution_id, false, "Installation timeout");
    }
    
    // Cleanup
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Cleaning up temporary files"));
    if (remove(bundle_path.c_str()) == 0) {
        DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Temporary bundle file removed"));
    } else {
        DLT_LOG(updateContext, DLT_LOG_WARN, DLT_STRING("Failed to remove temporary bundle file"));
    }
    
    update_in_progress = false;
    current_execution_id.clear();
    
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update process completed"));
    return true;
}

int main() {
    // Initialize DLT
    DLT_REGISTER_APP(DLT_APP_NAME.c_str(), "RAUC Hawkbit C++ Client");
    DLT_REGISTER_CONTEXT(hawkbitContext, DLT_HAWK_CONTEXT.c_str(), "Hawkbit client context");
    DLT_REGISTER_CONTEXT(raucContext, DLT_RAUC_CONTEXT.c_str(), "RAUC client context");
    DLT_REGISTER_CONTEXT(updateContext, DLT_UPDATE_CONTEXT.c_str(), "Update process context");
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("=== RAUC Hawkbit C++ Client Starting ==="));
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Server URL: "), DLT_STRING(HAWKBIT_SERVER_URL.c_str()));
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Tenant: "), DLT_STRING(HAWKBIT_TENANT.c_str()));
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Controller ID: "), DLT_STRING(HAWKBIT_CONTROLLER_ID.c_str()));
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Poll interval: "), DLT_INT(POLL_INTERVAL_SECONDS), DLT_STRING(" seconds"));
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize Hawkbit client
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Initializing Hawkbit client"));
    HawkbitClient hawkbitClient(HAWKBIT_SERVER_URL, HAWKBIT_TENANT, HAWKBIT_CONTROLLER_ID);
    
    // Initialize RAUC client
    DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("Initializing RAUC client"));
    RaucClient raucClient;
    if (!raucClient.connect()) {
        DLT_LOG(raucContext, DLT_LOG_ERROR, DLT_STRING("Failed to connect to RAUC DBus service"));
        DLT_UNREGISTER_CONTEXT(hawkbitContext);
        DLT_UNREGISTER_CONTEXT(raucContext);
        DLT_UNREGISTER_CONTEXT(updateContext);
        DLT_UNREGISTER_APP();
        return 1;
    }
    
    DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("Successfully connected to RAUC DBus service"));
    
    // Set up RAUC callbacks
    raucClient.setProgressCallback(onRaucProgress);
    raucClient.setCompletedCallback(onRaucCompleted);
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Starting main polling loop"));
    
    int poll_counter = 0;
    while (running) {
        poll_counter++;
        printf("Polling Hawkbit server (attempt %d) - Poll interval: %d seconds\n", poll_counter, POLL_INTERVAL_SECONDS);
        fflush(stdout);
        DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Polling Hawkbit server (attempt "), DLT_INT(poll_counter), DLT_STRING(")"));
        
        // Don't poll if an update is in progress
        if (update_in_progress) {
            DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update in progress, skipping poll"));
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }
        
        // Poll Hawkbit for updates
        std::string response;
        if (hawkbitClient.pollForUpdates()) {
            DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Successfully polled Hawkbit server"));
            
            // Parse the response to check for actual updates
            UpdateInfo update_info;
            if (hawkbitClient.parseUpdateResponse(response, update_info)) {
                DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update available detected"));
                DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
                DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Version: "), DLT_STRING(update_info.version.c_str()));
                
                // Perform the update
                if (!performUpdate(hawkbitClient, raucClient, update_info)) {
                    DLT_LOG(updateContext, DLT_LOG_ERROR, DLT_STRING("Update process failed"));
                }
            } else {
                DLT_LOG(hawkbitContext, DLT_LOG_DEBUG, DLT_STRING("No update available in response"));
            }
        } else {
            DLT_LOG(hawkbitContext, DLT_LOG_ERROR, DLT_STRING("Failed to poll Hawkbit server"));
        }
        
        // Wait before next poll
        DLT_LOG(hawkbitContext, DLT_LOG_DEBUG, DLT_STRING("Waiting "), DLT_INT(POLL_INTERVAL_SECONDS), DLT_STRING(" seconds before next poll"));
        for (int i = 0; i < POLL_INTERVAL_SECONDS && running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("=== RAUC Hawkbit C++ Client Stopping ==="));
    
    // Cleanup
    if (update_in_progress) {
        DLT_LOG(updateContext, DLT_LOG_WARN, DLT_STRING("Update was in progress during shutdown"));
    }
    
    raucClient.disconnect();
    
    // Unregister DLT
    DLT_UNREGISTER_CONTEXT(hawkbitContext);
    DLT_UNREGISTER_CONTEXT(raucContext);
    DLT_UNREGISTER_CONTEXT(updateContext);
    DLT_UNREGISTER_APP();
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("RAUC Hawkbit C++ Client stopped gracefully"));
    
    return 0;
} 