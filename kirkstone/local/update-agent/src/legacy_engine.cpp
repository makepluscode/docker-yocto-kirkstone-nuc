#include "legacy_engine.h"

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_engine);
#endif

LegacyEngine::LegacyEngine()
    : initialized_(false)
    , installing_(false)
    , last_error_("")
    , current_operation_("idle")
    , current_progress_(ProgressInfo())
    , progress_callback_(nullptr)
    , completed_callback_(nullptr)
    , system_slots_(nullptr)
    , config_file_path_("")
    , system_compatible_("")
{
#ifdef WITH_DLT
    DLT_REGISTER_CONTEXT(dlt_context_engine, "LENG", "Legacy Engine");
#endif
    logInfo("LegacyEngine constructor called");
}

LegacyEngine::~LegacyEngine() {
    if (initialized_) {
        // 정리 작업
        r_context_cleanup();
    }

#ifdef WITH_DLT
    DLT_UNREGISTER_CONTEXT(dlt_context_engine);
#endif
}

bool LegacyEngine::initialize(const std::string& config_file_path) {
    if (initialized_) {
        return true;
    }

    config_file_path_ = config_file_path;
    logInfo("Starting Legacy engine initialization with config: " + config_file_path_);

    // RAUC 컨텍스트 초기화
    logInfo("Initializing RAUC context...");

    // Check if config file exists
    if (access(config_file_path_.c_str(), R_OK) != 0) {
        last_error_ = "RAUC config file not accessible: " + config_file_path_;
        logError(last_error_);
        return false;
    }
    logInfo("RAUC config file is accessible");

    if (!r_context_init()) {
        last_error_ = "Failed to initialize RAUC context";
        logError(last_error_);
        return false;
    }
    logInfo("RAUC context initialized successfully");

    // 설정 파일 로드
    logInfo("Loading system configuration...");
    if (!loadSystemConfig()) {
        last_error_ = "Failed to load system configuration";
        logError(last_error_);
        r_context_cleanup();
        return false;
    }
    logInfo("System configuration loaded successfully");

    // 슬롯 상태 파악
    logInfo("Determining slot states...");
    if (!determineSlotStates()) {
        last_error_ = "Failed to determine slot states";
        logError(last_error_);
        r_context_cleanup();
        return false;
    }
    logInfo("Slot states determined successfully");

    // 부트 상태 파악
    logInfo("Determining boot states...");
    if (!determineBootStates()) {
        last_error_ = "Failed to determine boot states";
        logError(last_error_);
        r_context_cleanup();
        return false;
    }
    logInfo("Boot states determined successfully");

    initialized_ = true;
    current_operation_ = "idle";

    logInfo("Legacy Engine initialized successfully");
    return true;
}

bool LegacyEngine::installPackage(const std::string& package_path,
                              ProgressCallback progress_callback,
                              CompletedCallback completed_callback) {
    if (!initialized_) {
        last_error_ = "Legacy Engine not initialized";
        logError(last_error_);
        return false;
    }

    if (installing_) {
        last_error_ = "Installation already in progress";
        logError(last_error_);
        return false;
    }

    // 콜백 저장
    progress_callback_ = progress_callback;
    completed_callback_ = completed_callback;

    installing_ = true;
    current_operation_ = "installing";
    current_progress_ = ProgressInfo(0, "Starting installation", 0);

    // 설치 시작
    bool success = doInstallPackage(package_path);

    if (!success) {
        installing_ = false;
        current_operation_ = "idle";

        // 완료 콜백 호출
        if (completed_callback_) {
            completed_callback_(InstallResult::FAILURE, last_error_);
        }

        return false;
    }

    return true;
}

std::vector<SlotInfo> LegacyEngine::getSlotStatus() {
    if (!initialized_) {
        logError("Legacy Engine not initialized");
        return {};
    }

    GHashTable* slots = r_context_get_system_slots();
    if (!slots) {
        logError("No system slots available");
        return {};
    }

    return convertSlotsToVector(slots);
}

std::string LegacyEngine::getBootSlot() {
    if (!initialized_) {
        logError("Legacy Engine not initialized");
        return "";
    }

    const gchar* bootslot = r_context_get_bootslot();
    return bootslot ? std::string(bootslot) : "";
}

std::string LegacyEngine::getCompatible() {
    if (!initialized_) {
        logError("Legacy Engine not initialized");
        return "";
    }

    const gchar* compatible = r_context_get_compatible();
    return compatible ? std::string(compatible) : "";
}

ProgressInfo LegacyEngine::getCurrentProgress() {
    return current_progress_;
}

std::string LegacyEngine::getLastError() {
    return last_error_;
}

std::string LegacyEngine::getOperation() {
    return current_operation_;
}

bool LegacyEngine::getPackageInfo(const std::string& package_path,
                                std::string& compatible,
                                std::string& version) {
    if (!initialized_) {
        last_error_ = "Legacy Engine not initialized";
        logError(last_error_);
        return false;
    }

    GError* error = nullptr;
    gchar* compat_str = nullptr;
    gchar* version_str = nullptr;

    // 패키지 정보 가져오기
    gboolean success = r_bundle_get_info(package_path.c_str(), &compat_str, &version_str, &error);

    if (!success || error) {
        if (error) {
            last_error_ = std::string("Failed to get bundle info: ") + error->message;
            g_error_free(error);
        } else {
            last_error_ = "Failed to get bundle info: Unknown error";
        }
        logError(last_error_);

        if (compat_str) g_free(compat_str);
        if (version_str) g_free(version_str);
        return false;
    }

    compatible = compat_str ? std::string(compat_str) : "";
    version = version_str ? std::string(version_str) : "";

    if (compat_str) g_free(compat_str);
    if (version_str) g_free(version_str);

    return true;
}

bool LegacyEngine::isInstalling() const {
    return installing_;
}

bool LegacyEngine::isInitialized() const {
    return initialized_;
}

// Private 메서드들

bool LegacyEngine::loadSystemConfig() {
    GError* error = nullptr;
    logInfo("Loading config file: " + config_file_path_);

    // 설정 파일 로드
    gboolean success = load_config_file(config_file_path_.c_str(), &error);

    if (!success || error) {
        if (error) {
            last_error_ = std::string("Failed to load config: ") + error->message;
            logError("Config load error: " + last_error_);
            g_error_free(error);
        } else {
            last_error_ = "Failed to load config: Unknown error";
            logError("Config load failed with unknown error");
        }
        return false;
    }
    logInfo("Config file loaded successfully");

    // 호환성 문자열 저장
    const gchar* compatible = r_context_get_compatible();
    if (compatible) {
        system_compatible_ = std::string(compatible);
        logInfo("System compatible: " + system_compatible_);
    } else {
        logError("Failed to get system compatible string");
        return false;
    }

    return true;
}

bool LegacyEngine::determineSlotStates() {
    GError* error = nullptr;

    // 슬롯 상태 결정
    gboolean success = determine_slot_states(&error);

    if (!success || error) {
        if (error) {
            last_error_ = std::string("Failed to determine slot states: ") + error->message;
            g_error_free(error);
        } else {
            last_error_ = "Failed to determine slot states: Unknown error";
        }
        return false;
    }

    return true;
}

bool LegacyEngine::determineBootStates() {
    GError* error = nullptr;

    // 부트 상태 결정
    gboolean success = determine_boot_states(&error);

    if (!success || error) {
        if (error) {
            last_error_ = std::string("Failed to determine boot states: ") + error->message;
            g_error_free(error);
        } else {
            last_error_ = "Failed to determine boot states: Unknown error";
        }
        return false;
    }

    return true;
}

bool LegacyEngine::checkPackage(const std::string& package_path, RaucBundle** bundle) {
    GError* error = nullptr;

    // 번들 검증 (서명 검증 포함)
    gboolean success = check_bundle(package_path.c_str(), bundle, CHECK_BUNDLE_DEFAULT, nullptr, &error);

    if (!success || error) {
        if (error) {
            last_error_ = std::string("Bundle check failed: ") + error->message;
            g_error_free(error);
        } else {
            last_error_ = "Bundle check failed: Unknown error";
        }
        return false;
    }

    return true;
}

bool LegacyEngine::doInstallPackage(const std::string& package_path) {
    GError* error = nullptr;

    // 설치 인자 생성
    RaucInstallArgs* args = install_args_new();
    if (!args) {
        last_error_ = "Failed to create install arguments";
        return false;
    }

    args->bundle_path = g_strdup(package_path.c_str());
    args->progress_callback = onProgressUpdate;
    args->completed_callback = onInstallCompleted;
    args->user_data = this;

    // 설치 실행
    gboolean success = install_run_simple(package_path.c_str(),
                                         onProgressUpdate,
                                         onInstallCompleted,
                                         this,
                                         &error);

    if (!success || error) {
        if (error) {
            last_error_ = std::string("Installation failed: ") + error->message;
            g_error_free(error);
        } else {
            last_error_ = "Installation failed: Unknown error";
        }

        install_args_free(args);
        return false;
    }

    // args는 설치 스레드에서 해제됨
    return true;
}

void LegacyEngine::onProgressUpdate(int percentage, const char* message, int nesting_depth, void* user_data) {
    LegacyEngine* engine = static_cast<LegacyEngine*>(user_data);
    if (!engine) return;

    engine->current_progress_ = ProgressInfo(percentage, message ? message : "", 0);

    if (engine->progress_callback_) {
        engine->progress_callback_(engine->current_progress_);
    }

    // 상세한 DLT 로깅 추가
    engine->logInfo("Installation progress: " + std::to_string(percentage) + "% - " +
                    (message ? message : ""));
}

void LegacyEngine::onInstallCompleted(RInstallResult result, const char* message, void* user_data) {
    LegacyEngine* engine = static_cast<LegacyEngine*>(user_data);
    if (!engine) return;

    engine->installing_ = false;
    engine->current_operation_ = "idle";

    InstallResult install_result = (result == 0) ? InstallResult::SUCCESS : InstallResult::FAILURE;
    std::string result_message = message ? message : "";

    if (install_result == InstallResult::SUCCESS) {
        engine->current_progress_ = ProgressInfo(100, "Installation completed successfully", 0);
        engine->logInfo("Installation completed successfully");
    } else {
        engine->last_error_ = result_message;
        engine->logError("Installation failed: " + result_message);
    }

    if (engine->completed_callback_) {
        engine->completed_callback_(install_result, result_message);
    }
}

std::vector<SlotInfo> LegacyEngine::convertSlotsToVector(GHashTable* slots) {
    std::vector<SlotInfo> slot_list;

    if (!slots) {
        return slot_list;
    }

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        const gchar* slot_name = static_cast<const gchar*>(key);
        RaucSlot* slot = static_cast<RaucSlot*>(value);

        if (!slot_name || !slot) {
            continue;
        }

        SlotInfo info(slot_name);

        // 슬롯 속성들 추가
        if (slot->description) {
            info.properties["description"] = slot->description;
        }
        if (slot->device) {
            info.properties["device"] = slot->device;
        }
        if (slot->type) {
            info.properties["type"] = slot->type;
        }
        if (slot->bootname) {
            info.properties["bootname"] = slot->bootname;
        }
        if (slot->sclass) {
            info.properties["class"] = slot->sclass;
        }

        // 상태 정보
        info.properties["state"] = r_slot_slotstate_to_str(slot->state);
        info.properties["bootable"] = slot->boot_good ? "true" : "false";

        // 마운트 포인트
        if (slot->mount_point) {
            info.properties["mount_point"] = slot->mount_point;
        }

        // 슬롯 상태 정보
        if (slot->status) {
            if (slot->status->bundle_compatible) {
                info.properties["bundle.compatible"] = slot->status->bundle_compatible;
            }
            if (slot->status->bundle_version) {
                info.properties["bundle.version"] = slot->status->bundle_version;
            }
            if (slot->status->bundle_description) {
                info.properties["bundle.description"] = slot->status->bundle_description;
            }
            if (slot->status->installed_timestamp) {
                info.properties["installed.timestamp"] = slot->status->installed_timestamp;
            }
            info.properties["installed.count"] = std::to_string(slot->status->installed_count);
        }

        slot_list.push_back(info);
    }

    return slot_list;
}

void LegacyEngine::logInfo(const std::string& message) {
#ifdef WITH_DLT
    DLT_LOG(dlt_context_engine, DLT_LOG_INFO, DLT_CSTRING(message.c_str()));
#else
    g_info("LegacyEngine: %s", message.c_str());
#endif
}

void LegacyEngine::logError(const std::string& message) {
#ifdef WITH_DLT
    DLT_LOG(dlt_context_engine, DLT_LOG_ERROR, DLT_CSTRING(message.c_str()));
#else
    g_critical("LegacyEngine: %s", message.c_str());
#endif
}

void LegacyEngine::logDebug(const std::string& message) {
#ifdef WITH_DLT
    DLT_LOG(dlt_context_engine, DLT_LOG_DEBUG, DLT_CSTRING(message.c_str()));
#else
    g_debug("LegacyEngine: %s", message.c_str());
#endif
}
