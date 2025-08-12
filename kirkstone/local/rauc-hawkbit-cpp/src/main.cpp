#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <dlt/dlt.h>
#include "hawkbit_client.h"
#include "rauc_client.h"

DLT_DECLARE_CONTEXT(hawkbitContext);
DLT_DECLARE_CONTEXT(raucContext);

volatile bool running = true;

// Configuration - these should be configurable via environment variables or config file
const std::string HAWKBIT_SERVER_URL = "https://hawkbit.example.com";
const std::string HAWKBIT_TENANT = "DEFAULT";
const std::string HAWKBIT_CONTROLLER_ID = "nuc-device-001";
const int POLL_INTERVAL_SECONDS = 30;

void signalHandler(int signal) {
    running = false;
}

void onRaucProgress(int progress) {
    DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("RAUC installation progress: "), DLT_INT(progress), DLT_STRING("%"));
}

void onRaucCompleted(bool success, const std::string& message) {
    if (success) {
        DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("RAUC installation completed successfully: "), DLT_STRING(message.c_str()));
    } else {
        DLT_LOG(raucContext, DLT_LOG_ERROR, DLT_STRING("RAUC installation failed: "), DLT_STRING(message.c_str()));
    }
}

int main() {
    // Initialize DLT
    DLT_REGISTER_APP("RHCP", "RAUC Hawkbit C++ Client");
    DLT_REGISTER_CONTEXT(hawkbitContext, "HAWK", "Hawkbit client context");
    DLT_REGISTER_CONTEXT(raucContext, "RAUC", "RAUC client context");
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("RAUC Hawkbit C++ client started"));
    
    // Initialize Hawkbit client
    HawkbitClient hawkbitClient(HAWKBIT_SERVER_URL, HAWKBIT_TENANT, HAWKBIT_CONTROLLER_ID);
    
    // Initialize RAUC client
    RaucClient raucClient;
    if (!raucClient.connect()) {
        DLT_LOG(raucContext, DLT_LOG_ERROR, DLT_STRING("Failed to connect to RAUC DBus service"));
        return 1;
    }
    
    // Set up RAUC callbacks
    raucClient.setProgressCallback(onRaucProgress);
    raucClient.setCompletedCallback(onRaucCompleted);
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Connected to RAUC service, starting polling loop"));
    
    while (running) {
        DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Polling Hawkbit server for updates"));
        
        // Poll Hawkbit for updates
        if (hawkbitClient.pollForUpdates()) {
            DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Successfully polled Hawkbit server"));
            
            // TODO: Parse the response to check for actual updates
            // For now, we'll just log that we're polling
            
            // Example of how to handle an update (commented out for now)
            /*
            if (update_available) {
                std::string bundle_path = "/tmp/update.raucb";
                if (hawkbitClient.downloadBundle(download_url, bundle_path)) {
                    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("Bundle downloaded, starting installation"));
                    
                    if (raucClient.installBundle(bundle_path)) {
                        DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("Bundle installation started"));
                        
                        // Wait for installation to complete
                        std::string status;
                        while (raucClient.getStatus(status) && status == "installing") {
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }
                        
                        if (status == "idle") {
                            DLT_LOG(raucContext, DLT_LOG_INFO, DLT_STRING("Installation completed successfully"));
                            hawkbitClient.sendFeedback(execution_id, "success");
                        } else {
                            DLT_LOG(raucContext, DLT_LOG_ERROR, DLT_STRING("Installation failed"));
                            hawkbitClient.sendFeedback(execution_id, "error", "Installation failed");
                        }
                    } else {
                        DLT_LOG(raucContext, DLT_LOG_ERROR, DLT_STRING("Failed to start bundle installation"));
                        hawkbitClient.sendFeedback(execution_id, "error", "Failed to start installation");
                    }
                } else {
                    DLT_LOG(hawkbitContext, DLT_LOG_ERROR, DLT_STRING("Failed to download bundle"));
                    hawkbitClient.sendFeedback(execution_id, "error", "Download failed");
                }
            }
            */
        } else {
            DLT_LOG(hawkbitContext, DLT_LOG_ERROR, DLT_STRING("Failed to poll Hawkbit server"));
        }
        
        // Wait before next poll
        for (int i = 0; i < POLL_INTERVAL_SECONDS && running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    DLT_LOG(hawkbitContext, DLT_LOG_INFO, DLT_STRING("RAUC Hawkbit C++ client stopping"));
    
    // Cleanup
    raucClient.disconnect();
    
    // Unregister DLT
    DLT_UNREGISTER_CONTEXT(hawkbitContext);
    DLT_UNREGISTER_CONTEXT(raucContext);
    DLT_UNREGISTER_APP();
    
    return 0;
} 