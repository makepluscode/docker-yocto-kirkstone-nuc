#include <dlt/dlt.h>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <cmath>
#include "agent.h"
#include "updater.h"
#include "config.h"

// DLT context
DLT_DECLARE_CONTEXT(dlt_context_main);

// Global variables
volatile bool running = true;
volatile bool update_in_progress = false;

// Signal handler
void signalHandler(int signum) {
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Received signal "), DLT_INT(signum), DLT_STRING(", shutting down..."));
    running = false;
}

// Update progress callback
void onUpdateProgress(int progress) {
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update progress: "), DLT_INT(progress), DLT_STRING("%"));
}

// Update completed callback
void onUpdateCompleted(bool success, const std::string& message) {
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update completed: "), DLT_STRING(success ? "success" : "failure"), DLT_STRING(" - "), DLT_STRING(message.c_str()));
    update_in_progress = false;
}

// Perform update process
bool performUpdate(Agent& agent, Updater& updater, const UpdateInfo& update_info) {
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("=== Starting update process ==="));
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Version: "), DLT_STRING(update_info.version.c_str()));
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Download URL: "), DLT_STRING(update_info.download_url.c_str()));

    update_in_progress = true;

    // Send started feedback
    if (!agent.sendStartedFeedback(update_info.execution_id)) {
        DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to send started feedback"));
        update_in_progress = false;
        return false;
    }

    // Send initial progress - starting download (5%)
    agent.sendProgressFeedback(update_info.execution_id, 5, "Starting bundle download...");

    // Download bundle
    std::string local_path = UPDATE_BUNDLE_PATH;
    if (!agent.downloadBundle(update_info.download_url, local_path, update_info.expected_size)) {
        DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to download bundle"));
        agent.sendFinishedFeedback(update_info.execution_id, false, "Download failed");
        update_in_progress = false;
        return false;
    }

    // Send progress feedback - download completed (30%)
    agent.sendProgressFeedback(update_info.execution_id, 30, "Bundle downloaded successfully");

    // Send progress - starting installation (35%)
    agent.sendProgressFeedback(update_info.execution_id, 35, "Starting RAUC installation...");

    // Install bundle using RAUC (non-blocking)
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Starting RAUC installation..."));
    if (!updater.installBundle(local_path)) {
        DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to start bundle installation"));
        agent.sendFinishedFeedback(update_info.execution_id, false, "Installation failed to start");
        update_in_progress = false;
        return false;
    }

    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("RAUC installation started, waiting for completion..."));

    // Wait for installation to complete (with timeout)
    int timeout_counter = 0;
    const int MAX_TIMEOUT = INSTALLATION_TIMEOUT_SECONDS_MAIN; // Installation timeout from config
    bool installation_completed = false;
    bool installation_success = false;

    while (update_in_progress && timeout_counter < MAX_TIMEOUT) {
        std::this_thread::sleep_for(std::chrono::seconds(MAIN_LOOP_SLEEP_SECONDS));
        timeout_counter += MAIN_LOOP_SLEEP_SECONDS;

        // Check RAUC status based on config interval
        if (timeout_counter % RAUC_STATUS_CHECK_INTERVAL_SECONDS == 0) {
            std::string status;
            if (updater.getStatus(status)) {
                DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("RAUC status: "), DLT_STRING(status.c_str()));
                if (status == "idle") {
                    installation_completed = true;
                    installation_success = true;
                    break;
                } else if (status == "failed") {
                    installation_completed = true;
                    installation_success = false;
                    break;
                }
            } else {
                DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to get RAUC status"));
            }
        }

        // Send progress feedback based on config interval with improved algorithm
        if (timeout_counter % PROGRESS_FEEDBACK_INTERVAL_SECONDS == 0) {
            // Progress from 40% (installation started) to 95% (installation almost done)
            // Use logarithmic curve for more realistic progress: fast initial progress, slower near end
            float elapsed_ratio = (float)timeout_counter / MAX_TIMEOUT;
            
            // Apply logarithmic curve: progress = 40 + 55 * (1 - e^(-3*elapsed_ratio))
            float progress_factor = 1.0f - expf(-3.0f * elapsed_ratio);
            int progress = 40 + (int)(55.0f * progress_factor);
            
            // Cap at 95% until completion
            if (progress > 95) progress = 95;
            
            // Add more descriptive messages based on progress
            std::string message;
            if (progress < 60) {
                message = "Installing bundle - extracting files...";
            } else if (progress < 80) {
                message = "Installing bundle - updating partitions...";
            } else {
                message = "Installing bundle - finalizing installation...";
            }
            
            agent.sendProgressFeedback(update_info.execution_id, progress, message);
        }
    }

    if (timeout_counter >= MAX_TIMEOUT) {
        DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Installation timeout after "), DLT_INT(MAX_TIMEOUT), DLT_STRING(" seconds"));
        agent.sendFinishedFeedback(update_info.execution_id, false, "Installation timeout");
        update_in_progress = false;
        return false;
    }

    if (installation_success) {
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Installation completed successfully"));
        agent.sendProgressFeedback(update_info.execution_id, 100, "Installation completed");
        agent.sendFinishedFeedback(update_info.execution_id, true, "Update completed successfully");
        
        // Clean up downloaded file
        if (remove(local_path.c_str()) == 0) {
            DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Cleaned up downloaded bundle file"));
        } else {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to clean up downloaded bundle file"));
        }
        
        // Reboot system to boot into new image
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update completed successfully. Rebooting system to new image..."));
        std::this_thread::sleep_for(std::chrono::seconds(REBOOT_DELAY_SECONDS)); // Brief delay for log message
        system("sync && reboot");
        running = false;
    } else {
        DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Installation failed"));
        agent.sendFinishedFeedback(update_info.execution_id, false, "Installation failed");
        
        // Clean up downloaded file on failure too
        if (remove(local_path.c_str()) == 0) {
            DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Cleaned up downloaded bundle file after failure"));
        }
    }

    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("=== Update process completed ==="));
    update_in_progress = false;
    
    // Add a small delay to ensure all cleanup is complete
    std::this_thread::sleep_for(std::chrono::milliseconds(CLEANUP_DELAY_MS));
    
    return installation_success;
}

int main() {
    DLT_REGISTER_APP("UAGT", "Update Agent");
    DLT_REGISTER_CONTEXT(dlt_context_main, "MAIN", "Main Application Logic");
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("=== Update Agent Starting ==="));

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize update agent
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Initializing update agent"));
    Agent agent(UPDATE_SERVER_URL, UPDATE_TENANT, DEVICE_ID);

    // Initialize updater
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Initializing updater"));
    Updater updater;
    if (!updater.connect()) {
        DLT_LOG(dlt_context_main, DLT_LOG_FATAL, DLT_STRING("Failed to connect to RAUC DBus service"));
        return 1;
    }

    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Successfully connected to RAUC DBus service"));

    // Set up RAUC callbacks
    updater.setProgressCallback(onUpdateProgress);
    updater.setCompletedCallback(onUpdateCompleted);

    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Starting main polling loop"));

    int poll_counter = 0;
    while (running) {
        poll_counter++;
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Polling update server (attempt "), DLT_INT(poll_counter), DLT_STRING(")"));

        // Don't poll if an update is in progress
        if (update_in_progress) {
            DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update in progress, skipping poll"));
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // Poll Hawkbit for updates
        std::string response;
        if (agent.pollForUpdates(response)) {
            DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Successfully polled update server"));

            // Parse the response to check for actual updates
            UpdateInfo update_info;
            if (agent.parseUpdateResponse(response, update_info)) {
                DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update available detected"));
                DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
                DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Version: "), DLT_STRING(update_info.version.c_str()));

                // Perform the update
                if (!performUpdate(agent, updater, update_info)) {
                    DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Update process failed"));
                }
            } else {
                DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("No update available in response"));
            }
        } else {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to poll update server"));
        }

        // Wait before next poll
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Waiting "), DLT_INT(POLL_INTERVAL_SECONDS), DLT_STRING(" seconds before next poll"));
        for (int i = 0; i < POLL_INTERVAL_SECONDS && running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("=== Update Agent Stopping ==="));

    // Cleanup
    if (update_in_progress) {
        DLT_LOG(dlt_context_main, DLT_LOG_WARN, DLT_STRING("Update was in progress during shutdown"));
    }

    updater.disconnect();

    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update Agent stopped gracefully"));
    DLT_UNREGISTER_CONTEXT(dlt_context_main);
    DLT_UNREGISTER_APP();

    return 0;
}