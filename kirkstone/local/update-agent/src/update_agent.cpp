#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include "agent.h"
#include "updater.h"
#include "config.h"

// Global variables
volatile bool running = true;
volatile bool update_in_progress = false;

// Signal handler
void signalHandler(int signum) {
    std::cout << "Received signal " << signum << ", shutting down..." << std::endl;
    running = false;
}

// Update progress callback
void onUpdateProgress(int progress) {
    std::cout << "Update progress: " << progress << "%" << std::endl;
}

// Update completed callback
void onUpdateCompleted(bool success, const std::string& message) {
    std::cout << "Update completed: " << (success ? "success" : "failure") << " - " << message << std::endl;
    update_in_progress = false;
}

// Perform update process
bool performUpdate(Agent& agent, Updater& updater, const UpdateInfo& update_info) {
    std::cout << "=== Starting update process ===" << std::endl;
    std::cout << "Execution ID: " << update_info.execution_id << std::endl;
    std::cout << "Version: " << update_info.version << std::endl;
    std::cout << "Download URL: " << update_info.download_url << std::endl;

    update_in_progress = true;

    // Send started feedback
    if (!agent.sendStartedFeedback(update_info.execution_id)) {
        std::cout << "Failed to send started feedback" << std::endl;
        update_in_progress = false;
        return false;
    }

    // Download bundle
    std::string local_path = UPDATE_BUNDLE_PATH;
    if (!agent.downloadBundle(update_info.download_url, local_path, update_info.expected_size)) {
        std::cout << "Failed to download bundle" << std::endl;
        agent.sendFinishedFeedback(update_info.execution_id, false, "Download failed");
        update_in_progress = false;
        return false;
    }

    // Send progress feedback
    agent.sendProgressFeedback(update_info.execution_id, 50, "Bundle downloaded successfully");

    // Install bundle using RAUC (non-blocking)
    std::cout << "Starting RAUC installation..." << std::endl;
    if (!updater.installBundle(local_path)) {
        std::cout << "Failed to start bundle installation" << std::endl;
        agent.sendFinishedFeedback(update_info.execution_id, false, "Installation failed to start");
        update_in_progress = false;
        return false;
    }

    std::cout << "RAUC installation started, waiting for completion..." << std::endl;

    // Wait for installation to complete (with timeout)
    int timeout_counter = 0;
    const int MAX_TIMEOUT = 300; // 5 minutes timeout
    bool installation_completed = false;
    bool installation_success = false;

    while (update_in_progress && timeout_counter < MAX_TIMEOUT) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        timeout_counter += 2;

        // Check RAUC status every 10 seconds
        if (timeout_counter % 10 == 0) {
            std::string status;
            if (updater.getStatus(status)) {
                std::cout << "RAUC status: " << status << std::endl;
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
                std::cout << "Failed to get RAUC status" << std::endl;
            }
        }

        // Send progress feedback every 30 seconds
        if (timeout_counter % 30 == 0) {
            int progress = 50 + (timeout_counter * 50 / MAX_TIMEOUT);
            if (progress > 95) progress = 95;
            agent.sendProgressFeedback(update_info.execution_id, progress, "Installation in progress...");
        }
    }

    if (timeout_counter >= MAX_TIMEOUT) {
        std::cout << "Installation timeout after " << MAX_TIMEOUT << " seconds" << std::endl;
        agent.sendFinishedFeedback(update_info.execution_id, false, "Installation timeout");
        update_in_progress = false;
        return false;
    }

    if (installation_success) {
        std::cout << "Installation completed successfully" << std::endl;
        agent.sendProgressFeedback(update_info.execution_id, 100, "Installation completed");
        agent.sendFinishedFeedback(update_info.execution_id, true, "Update completed successfully");
        
        // Clean up downloaded file
        if (remove(local_path.c_str()) == 0) {
            std::cout << "Cleaned up downloaded bundle file" << std::endl;
        } else {
            std::cout << "Failed to clean up downloaded bundle file" << std::endl;
        }
        
        // Reboot system to boot into new image
        std::cout << "Update completed successfully. Rebooting system to new image..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Brief delay for log message
        system("sync && reboot");
        running = false;
    } else {
        std::cout << "Installation failed" << std::endl;
        agent.sendFinishedFeedback(update_info.execution_id, false, "Installation failed");
        
        // Clean up downloaded file on failure too
        if (remove(local_path.c_str()) == 0) {
            std::cout << "Cleaned up downloaded bundle file after failure" << std::endl;
        }
    }

    std::cout << "=== Update process completed ===" << std::endl;
    update_in_progress = false;
    
    // Add a small delay to ensure all cleanup is complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return installation_success;
}

int main() {
    std::cout << "=== Update Agent Starting ===" << std::endl;

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize update agent
    std::cout << "Initializing update agent" << std::endl;
    Agent agent(UPDATE_SERVER_URL, UPDATE_TENANT, DEVICE_ID);

    // Initialize updater
    std::cout << "Initializing updater" << std::endl;
    Updater updater;
    if (!updater.connect()) {
        std::cout << "Failed to connect to RAUC DBus service" << std::endl;
        return 1;
    }

    std::cout << "Successfully connected to RAUC DBus service" << std::endl;

    // Set up RAUC callbacks
    updater.setProgressCallback(onUpdateProgress);
    updater.setCompletedCallback(onUpdateCompleted);

    std::cout << "Starting main polling loop" << std::endl;

    int poll_counter = 0;
    while (running) {
        poll_counter++;
        printf("Polling update server (attempt %d) - Poll interval: %d seconds\n", poll_counter, POLL_INTERVAL_SECONDS);
        fflush(stdout);
        std::cout << "Polling update server (attempt " << poll_counter << ")" << std::endl;

        // Don't poll if an update is in progress
        if (update_in_progress) {
            std::cout << "Update in progress, skipping poll" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // Poll Hawkbit for updates
        std::string response;
        if (agent.pollForUpdates(response)) {
            std::cout << "Successfully polled update server" << std::endl;

            // Parse the response to check for actual updates
            UpdateInfo update_info;
            if (agent.parseUpdateResponse(response, update_info)) {
                std::cout << "Update available detected" << std::endl;
                std::cout << "Execution ID: " << update_info.execution_id << std::endl;
                std::cout << "Version: " << update_info.version << std::endl;

                // Perform the update
                if (!performUpdate(agent, updater, update_info)) {
                    std::cout << "Update process failed" << std::endl;
                }
            } else {
                std::cout << "No update available in response" << std::endl;
            }
        } else {
            std::cout << "Failed to poll update server" << std::endl;
        }

        // Wait before next poll
        std::cout << "Waiting " << POLL_INTERVAL_SECONDS << " seconds before next poll" << std::endl;
        for (int i = 0; i < POLL_INTERVAL_SECONDS && running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "=== Update Agent Stopping ===" << std::endl;

    // Cleanup
    if (update_in_progress) {
        std::cout << "Update was in progress during shutdown" << std::endl;
    }

    updater.disconnect();

    std::cout << "Update Agent stopped gracefully" << std::endl;

    return 0;
}