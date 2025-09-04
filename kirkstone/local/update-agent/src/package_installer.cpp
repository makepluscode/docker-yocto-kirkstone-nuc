#include <dlt/dlt.h>
#include <cstring>
#include <unistd.h> // For access()
#include "package_installer.h"

DLT_DECLARE_CONTEXT(dlt_context_updater);

PackageInstaller::PackageInstaller() : connected_(false) {
    DLT_REGISTER_CONTEXT(dlt_context_updater, "PKGI", "Package Installer - Update Library Client");
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Initializing Package Installer (Update Library Client)"));
}

PackageInstaller::~PackageInstaller() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Destroying Package Installer"));
    disconnect();
    DLT_UNREGISTER_CONTEXT(dlt_context_updater);
}

bool PackageInstaller::connect() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Connecting to update library"));

    // Create legacy engine instance
    update_client_ = std::make_unique<LegacyEngine>();

    // Initialize the engine
    if (!update_client_->initialize()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to initialize legacy engine: "), DLT_STRING(update_client_->getLastError().c_str()));
        update_client_.reset();
        return false;
    }

    connected_ = true;
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Successfully connected to update library"));
    return true;
}

void PackageInstaller::disconnect() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Disconnecting from update library"));
    if (update_client_) {
        update_client_.reset();
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update library client disconnected"));
    }
    connected_ = false;
}

bool PackageInstaller::isConnected() const {
    return connected_ && update_client_;
}

bool PackageInstaller::checkService() {
    if (!isConnected()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to update library"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Checking update library status..."));

    // Try to get current operation status to verify service is responding
    std::string status = update_client_->getOperation();
    if (!status.empty()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update library is responding, current status: "), DLT_STRING(status.c_str()));
        return true;
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Update library status check failed, attempting to reconnect..."));

        // Try to reconnect and retry once
        disconnect();
        if (connect()) {
            DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Reconnected successfully, retrying status check..."));
            status = update_client_->getOperation();
            if (!status.empty()) {
                DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update library is responding after reconnect, status: "), DLT_STRING(status.c_str()));
                return true;
            }
        }

        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Update library is not responding even after reconnect"));
        return false;
    }
}

bool PackageInstaller::installPackage(const std::string& package_path) {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Installing package: "), DLT_STRING(package_path.c_str()));

    // Check if file exists and is readable
    if (access(package_path.c_str(), F_OK) != 0) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Package file does not exist: "), DLT_STRING(package_path.c_str()));
        return false;
    }

    if (access(package_path.c_str(), R_OK) != 0) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Package file is not readable: "), DLT_STRING(package_path.c_str()));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Package file exists and is readable"));

    // Check update library status before attempting installation
    if (!checkService()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Update library is not available, cannot install package"));
        return false;
    }

    // Check if already installing
    if (update_client_->isInstalling()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Package installation already in progress"));
        return false;
    }

    // Set up callbacks for this installation
    auto progress_wrapper = [this](const ProgressInfo& progress) {
        if (progress_callback_) {
            progress_callback_(progress.percentage);
        }
        this->onProgressCallback(progress);
    };

    auto completed_wrapper = [this](InstallResult result, const std::string& message) {
        this->onCompletedCallback(result, message);
    };

    bool result = update_client_->installPackage(package_path, progress_wrapper, completed_wrapper);

    if (result) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Package installation started successfully"));
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Package installation failed to start: "), DLT_STRING(update_client_->getLastError().c_str()));
    }

    return result;
}

bool PackageInstaller::installPackageAsync(const std::string& package_path) {
    // For update-library, install is already async, so just call installPackage
    return installPackage(package_path);
}

bool PackageInstaller::getStatus(std::string& status) {
    if (!isConnected()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to update library"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting update library status"));

    status = update_client_->getOperation();
    if (!status.empty()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update library status: "), DLT_STRING(status.c_str()));
        return true;
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get status: "), DLT_STRING(update_client_->getLastError().c_str()));
        return false;
    }
}

bool PackageInstaller::getBootSlot(std::string& boot_slot) {
    if (!isConnected()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to update library"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting boot slot"));

    boot_slot = update_client_->getBootSlot();
    if (!boot_slot.empty()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Boot slot: "), DLT_STRING(boot_slot.c_str()));
        return true;
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get boot slot: "), DLT_STRING(update_client_->getLastError().c_str()));
        return false;
    }
}

bool PackageInstaller::markGood() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Marking current slot as good"));

    if (!isConnected()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to update library"));
        return false;
    }

    // Note: markGood/markBad functionality is not directly available in update-library
    // This would need to be implemented as a separate function in update-library
    // For now, we'll log a warning and return false
    DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("markGood functionality not yet implemented in update-library"));
    return false;
}

bool PackageInstaller::markBad() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Marking current slot as bad"));

    if (!isConnected()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to update library"));
        return false;
    }

    // Note: markGood/markBad functionality is not directly available in update-library
    // This would need to be implemented as a separate function in update-library
    // For now, we'll log a warning and return false
    DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("markBad functionality not yet implemented in update-library"));
    return false;
}

bool PackageInstaller::getPackageInfo(const std::string& package_path, std::string& info) {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting package info for: "), DLT_STRING(package_path.c_str()));

    if (!isConnected()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to update library"));
        return false;
    }

    std::string compatible, version;
    if (update_client_->getPackageInfo(package_path, compatible, version)) {
        info = "Compatible: " + compatible + ", Version: " + version;
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Bundle info: "), DLT_STRING(info.c_str()));
        return true;
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get bundle info: "), DLT_STRING(update_client_->getLastError().c_str()));
        return false;
    }
}

void PackageInstaller::setProgressCallback(std::function<void(int)> callback) {
    progress_callback_ = callback;
}

void PackageInstaller::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    completed_callback_ = callback;
}

void PackageInstaller::processMessages() {
    // For update-library, message processing is handled internally
    // This method is kept for compatibility but does nothing
    if (isConnected() && update_client_->isInstalling()) {
        // Check if installation is still in progress
        // The callbacks will be called automatically by update-library
    }
}

void PackageInstaller::onProgressCallback(const ProgressInfo& progress) {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Progress: "), DLT_INT(progress.percentage), DLT_STRING("% - "), DLT_STRING(progress.message.c_str()));

    if (progress_callback_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Calling progress callback"));
        progress_callback_(progress.percentage);
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("No progress callback registered"));
    }
}

void PackageInstaller::onCompletedCallback(InstallResult result, const std::string& message) {
    bool success = (result == InstallResult::SUCCESS);
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Installation completed: "), DLT_BOOL(success), DLT_STRING(" - "), DLT_STRING(message.c_str()));

    if (completed_callback_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Calling completed callback"));
        completed_callback_(success, message);
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("No completed callback registered"));
    }
}
