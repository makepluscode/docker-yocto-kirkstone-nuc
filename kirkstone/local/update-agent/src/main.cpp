#include "server_agent.h"
#include "service_agent.h"
#include "config.h"
#include <dlt/dlt.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <signal.h>
#include <atomic>

DLT_DECLARE_CONTEXT(dlt_context_main);

static std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Received signal: "), DLT_INT(signal));
    g_running = false;
}

class UpdateAgent {
public:
    UpdateAgent()
        : server_agent_(HOST_SERVER_URL, HOST_TENANT, DEVICE_ID),
          service_agent_() {

        DLT_REGISTER_CONTEXT(dlt_context_main, "MAIN", "Update Agent Main");
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Initializing Update Orchestrator"));

        // Set up ServiceAgent callbacks
        service_agent_.setProgressCallback([this](int progress) {
            handleInstallProgress(progress);
        });

        service_agent_.setCompletedCallback([this](bool success, const std::string& message) {
            handleInstallCompleted(success, message);
        });
    }

    ~UpdateAgent() {
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Shutting down Update Orchestrator"));
        service_agent_.disconnect();
        DLT_UNREGISTER_CONTEXT(dlt_context_main);
    }

    bool initialize() {
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Connecting to update service"));

        if (!service_agent_.connect()) {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to connect to update service"));
            return false;
        }

        if (!service_agent_.checkService()) {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Update service is not available"));
            return false;
        }

        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update Orchestrator initialized successfully"));
        return true;
    }

    void run() {
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Starting update agent main loop"));

        while (g_running) {
            try {
                // Process D-Bus messages to handle update-service signals
                service_agent_.processMessages();

                checkForUpdates();
                std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SECONDS));
            } catch (const std::exception& e) {
                DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Exception in main loop: "), DLT_STRING(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(30)); // Wait before retrying
            }
        }

        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update agent main loop ended"));
    }

private:
    ServerAgent server_agent_;
    ServiceAgent service_agent_;
    std::string current_execution_id_;
    bool installation_in_progress_ = false;

    void checkForUpdates() {
        if (installation_in_progress_) {
            DLT_LOG(dlt_context_main, DLT_LOG_DEBUG, DLT_STRING("Installation in progress, skipping poll"));
            return;
        }

        DLT_LOG(dlt_context_main, DLT_LOG_DEBUG, DLT_STRING("Polling for updates"));

        std::string response;
        if (!server_agent_.pollForUpdates(response)) {
            DLT_LOG(dlt_context_main, DLT_LOG_WARN, DLT_STRING("Failed to poll for updates"));
            return;
        }

        UpdateInfo update_info;
        if (!server_agent_.parseUpdateResponse(response, update_info)) {
            DLT_LOG(dlt_context_main, DLT_LOG_DEBUG, DLT_STRING("No updates available"));
            return;
        }

        if (!update_info.is_available) {
            DLT_LOG(dlt_context_main, DLT_LOG_DEBUG, DLT_STRING("No new updates"));
            return;
        }

        DLT_LOG(dlt_context_main, DLT_LOG_INFO,
                DLT_STRING("Update available - ID: "), DLT_STRING(update_info.execution_id.c_str()),
                DLT_STRING(", Version: "), DLT_STRING(update_info.version.c_str()));

        processUpdate(update_info);
    }

    void processUpdate(const UpdateInfo& update_info) {
        current_execution_id_ = update_info.execution_id;
        installation_in_progress_ = true;

        // Send started feedback
        server_agent_.sendStartedFeedback(current_execution_id_);

        // Download bundle
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Downloading bundle"));

        if (!server_agent_.downloadBundle(update_info.download_url, UPDATE_BUNDLE_PATH, update_info.expected_size)) {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to download bundle"));
            server_agent_.sendFinishedFeedback(current_execution_id_, false, "Download failed");
            installation_in_progress_ = false;
            return;
        }

        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Bundle downloaded successfully"));

        // Install bundle via ServiceAgent
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Starting bundle installation"));

        if (!service_agent_.installBundle(UPDATE_BUNDLE_PATH)) {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to start bundle installation"));
            server_agent_.sendFinishedFeedback(current_execution_id_, false, "Installation failed to start");
            installation_in_progress_ = false;
            return;
        }

        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Bundle installation started"));
        // Installation completion will be handled by callback
    }

    void handleInstallProgress(int progress) {
        DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Installation progress: "), DLT_INT(progress), DLT_STRING("%"));

        if (!current_execution_id_.empty()) {
            server_agent_.sendProgressFeedback(current_execution_id_, progress);
        }
    }

    void handleInstallCompleted(bool success, const std::string& message) {
        DLT_LOG(dlt_context_main, DLT_LOG_INFO,
                DLT_STRING("Installation completed - Success: "), DLT_BOOL(success),
                DLT_STRING(", Message: "), DLT_STRING(message.c_str()));

        if (!current_execution_id_.empty()) {
            server_agent_.sendFinishedFeedback(current_execution_id_, success, message);
            current_execution_id_.clear();
        }

        installation_in_progress_ = false;

        if (success) {
            DLT_LOG(dlt_context_main, DLT_LOG_INFO, DLT_STRING("Update completed successfully"));
        } else {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Update failed: "), DLT_STRING(message.c_str()));
        }
    }
};

int main() {
    // Initialize DLT
    DLT_REGISTER_APP("UAGT", "Update Agent");

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        UpdateAgent update_agent;

        if (!update_agent.initialize()) {
            DLT_LOG(dlt_context_main, DLT_LOG_ERROR, DLT_STRING("Failed to initialize update agent"));
            return 1;
        }

        update_agent.run();

    } catch (const std::exception& e) {
        DLT_LOG(dlt_context_main, DLT_LOG_FATAL, DLT_STRING("Fatal exception: "), DLT_STRING(e.what()));
        return 1;
    }

    DLT_UNREGISTER_APP();
    return 0;
}
