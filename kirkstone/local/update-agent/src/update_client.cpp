#include "update_client.h"
#include "rauc_engine.h"

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_client);
#endif

UpdateClient::UpdateClient()
    : rauc_engine_(nullptr)
    , completed_callback_(nullptr)
    , progress_callback_(nullptr)
    , error_callback_(nullptr)
    , initialized_(false)
    , installing_(false)
{
#ifdef WITH_DLT
    DLT_REGISTER_CONTEXT(dlt_context_client, "UCLI", "Update Client");
#endif

    rauc_engine_ = std::make_unique<RaucEngine>();
}

UpdateClient::~UpdateClient() {
#ifdef WITH_DLT
    DLT_UNREGISTER_CONTEXT(dlt_context_client);
#endif
}

bool UpdateClient::initialize(const std::string& config_file_path) {
    if (initialized_) {
        return true;
    }

    std::string config_path = config_file_path;
    if (config_path.empty()) {
        config_path = "/etc/rauc/system.conf";
    }

    if (!rauc_engine_->initialize(config_path)) {
        if (error_callback_) {
            error_callback_("Failed to initialize RAUC engine");
        }
        return false;
    }

    initialized_ = true;

#ifdef WITH_DLT
    DLT_LOG(dlt_context_client, DLT_LOG_INFO, DLT_CSTRING("UpdateClient initialized successfully"));
#else
    g_info("UpdateClient initialized successfully");
#endif

    return true;
}

bool UpdateClient::install(const std::string& bundle_path) {
    if (!initialized_) {
        if (error_callback_) {
            error_callback_("UpdateClient not initialized");
        }
        return false;
    }

    if (installing_) {
        if (error_callback_) {
            error_callback_("Installation already in progress");
        }
        return false;
    }

    installing_ = true;

    // 콜백 설정
    auto progress_wrapper = [this](const ProgressInfo& progress) {
        if (progress_callback_) {
            progress_callback_(progress);
        }
    };

    auto completed_wrapper = [this](InstallResult result, const std::string& message) {
        installing_ = false;
        if (completed_callback_) {
            completed_callback_(result, message);
        }
    };

    bool success = rauc_engine_->installBundle(bundle_path, progress_wrapper, completed_wrapper);

    if (!success) {
        installing_ = false;
        if (error_callback_) {
            error_callback_(rauc_engine_->getLastError());
        }
    }

#ifdef WITH_DLT
    DLT_LOG(dlt_context_client, DLT_LOG_INFO,
            DLT_CSTRING("Bundle installation"),
            DLT_CSTRING(success ? "started" : "failed"),
            DLT_CSTRING("for"),
            DLT_CSTRING(bundle_path.c_str()));
#else
    g_info("Bundle installation %s for %s", success ? "started" : "failed", bundle_path.c_str());
#endif

    return success;
}

std::vector<SlotInfo> UpdateClient::getSlotStatus() {
    if (!initialized_) {
        if (error_callback_) {
            error_callback_("UpdateClient not initialized");
        }
        return {};
    }

    return rauc_engine_->getSlotStatus();
}

std::string UpdateClient::getBootSlot() {
    if (!initialized_) {
        if (error_callback_) {
            error_callback_("UpdateClient not initialized");
        }
        return "";
    }

    return rauc_engine_->getBootSlot();
}

std::string UpdateClient::getCompatible() {
    if (!initialized_) {
        if (error_callback_) {
            error_callback_("UpdateClient not initialized");
        }
        return "";
    }

    return rauc_engine_->getCompatible();
}

bool UpdateClient::getBundleInfo(const std::string& bundle_path,
                                 std::string& compatible,
                                 std::string& version) {
    if (!initialized_) {
        if (error_callback_) {
            error_callback_("UpdateClient not initialized");
        }
        return false;
    }

    return rauc_engine_->getBundleInfo(bundle_path, compatible, version);
}

ProgressInfo UpdateClient::getCurrentProgress() {
    if (!initialized_) {
        if (error_callback_) {
            error_callback_("UpdateClient not initialized");
        }
        return ProgressInfo();
    }

    return rauc_engine_->getCurrentProgress();
}

std::string UpdateClient::getLastError() {
    if (!rauc_engine_) {
        return "RAUC engine not available";
    }

    return rauc_engine_->getLastError();
}

std::string UpdateClient::getOperation() {
    if (!initialized_) {
        return "not_initialized";
    }

    if (installing_) {
        return "installing";
    }

    return rauc_engine_->getOperation();
}

void UpdateClient::setCompletedCallback(CompletedCallback callback) {
    completed_callback_ = callback;
}

void UpdateClient::setProgressCallback(ProgressCallback callback) {
    progress_callback_ = callback;
}

void UpdateClient::setErrorCallback(ErrorCallback callback) {
    error_callback_ = callback;
}

bool UpdateClient::isInitialized() const {
    return initialized_;
}

bool UpdateClient::isInstalling() const {
    return installing_;
}
