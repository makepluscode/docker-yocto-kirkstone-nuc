
#include "hawkbit_client.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <dlt/dlt.h>

DLT_DECLARE_CONTEXT(hawkbitClientContext);

HawkbitClient::HawkbitClient(const std::string& server_url, const std::string& tenant, const std::string& controller_id)
    : server_url_(server_url), tenant_(tenant), controller_id_(controller_id), curl_handle_(nullptr) {
    DLT_REGISTER_CONTEXT(hawkbitClientContext, "HAWK", "Hawkbit client context");
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Initializing Hawkbit client"));
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Server URL: "), DLT_STRING(server_url_.c_str()));
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Tenant: "), DLT_STRING(tenant_.c_str()));
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Controller ID: "), DLT_STRING(controller_id_.c_str()));
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle_ = curl_easy_init();
    
    if (curl_handle_) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("CURL handle initialized successfully"));
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to initialize CURL handle"));
    }
}

HawkbitClient::~HawkbitClient() {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Cleaning up Hawkbit client"));
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("CURL handle cleaned up"));
    }
    curl_global_cleanup();
    DLT_UNREGISTER_CONTEXT(hawkbitClientContext);
}

size_t HawkbitClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t HawkbitClient::writeFileCallback(void* contents, size_t size, size_t nmemb, FILE* file) {
    return fwrite(contents, size, nmemb, file);
}

std::string HawkbitClient::buildPollUrl() const {
    std::string url = server_url_ + "/" + tenant_ + "/controller/v1/" + controller_id_;
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Built poll URL: "), DLT_STRING(url.c_str()));
    return url;
}

std::string HawkbitClient::buildFeedbackUrl(const std::string& execution_id) const {
    std::string url = server_url_ + "/" + tenant_ + "/controller/v1/" + controller_id_ + "/deploymentBase/" + execution_id + "/feedback";
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Built feedback URL: "), DLT_STRING(url.c_str()));
    return url;
}

bool HawkbitClient::pollForUpdates() {
    if (!curl_handle_) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("CURL handle not initialized"));
        return false;
    }

    std::string response;
    std::string url = buildPollUrl();

    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Polling for updates from: "), DLT_STRING(url.c_str()));

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SECONDS);
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, FOLLOW_REDIRECTS ? 1L : 0L);

    CURLcode res = curl_easy_perform(curl_handle_);
    if (res != CURLE_OK) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("curl_easy_perform() failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Poll response HTTP code: "), DLT_INT(http_code));

    if (http_code == 200) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Poll successful, response length: "), DLT_INT(response.length()));
        DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Poll response: "), DLT_STRING(response.c_str()));
        return true;
    } else if (http_code == 204) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("No updates available (HTTP 204)"));
        return true;
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool HawkbitClient::parseUpdateResponse(const std::string& response, UpdateInfo& update_info) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Parsing update response"));
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Response length: "), DLT_INT(response.length()));
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Response content: "), DLT_STRING(response.c_str()));
    
    update_info.is_available = false;
    
    if (response.empty()) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_WARN, DLT_STRING("Empty response received"));
        return false;
    }

    json_object* root = json_tokener_parse(response.c_str());
    if (!root) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to parse JSON response"));
        return false;
    }

    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("JSON parsed successfully"));

    // Check if there's a deployment
    json_object* deployment_obj;
    if (json_object_object_get_ex(root, "deployment", &deployment_obj)) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Deployment object found in response"));
        if (parseDeploymentInfo(deployment_obj, update_info)) {
            update_info.is_available = true;
            DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Update info parsed successfully"));
            DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
            DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Version: "), DLT_STRING(update_info.version.c_str()));
            DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Download URL: "), DLT_STRING(update_info.download_url.c_str()));
        } else {
            DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to parse deployment info"));
        }
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("No deployment object in response"));
    }

    json_object_put(root);
    return update_info.is_available;
}

bool HawkbitClient::parseDeploymentInfo(json_object* deployment_obj, UpdateInfo& update_info) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Parsing deployment info"));
    
    // Get execution ID
    json_object* execution_id_obj;
    if (json_object_object_get_ex(deployment_obj, "id", &execution_id_obj)) {
        update_info.execution_id = json_object_get_string(execution_id_obj);
        DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("No execution ID found in deployment"));
        return false;
    }

    // Get deployment info
    json_object* deployment_info_obj;
    if (json_object_object_get_ex(deployment_obj, "deployment", &deployment_info_obj)) {
        // Get version
        json_object* version_obj;
        if (json_object_object_get_ex(deployment_info_obj, "chunks", &version_obj)) {
            if (json_object_is_type(version_obj, json_type_array)) {
                int array_len = json_object_array_length(version_obj);
                DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found "), DLT_INT(array_len), DLT_STRING(" chunks"));
                if (array_len > 0) {
                    json_object* first_chunk = json_object_array_get_idx(version_obj, 0);
                    json_object* version_info_obj;
                    if (json_object_object_get_ex(first_chunk, "version", &version_info_obj)) {
                        update_info.version = json_object_get_string(version_info_obj);
                        DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found version: "), DLT_STRING(update_info.version.c_str()));
                    }
                }
            }
        }
    }

    // Get artifacts
    json_object* artifacts_obj;
    if (json_object_object_get_ex(deployment_obj, "artifacts", &artifacts_obj)) {
        if (json_object_is_type(artifacts_obj, json_type_array)) {
            int array_len = json_object_array_length(artifacts_obj);
            DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found "), DLT_INT(array_len), DLT_STRING(" artifacts"));
            if (array_len > 0) {
                json_object* first_artifact = json_object_array_get_idx(artifacts_obj, 0);
                if (parseArtifactInfo(first_artifact, update_info)) {
                    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Artifact info parsed successfully"));
                    return true;
                }
            }
        }
    }

    DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to parse deployment info completely"));
    return false;
}

bool HawkbitClient::parseArtifactInfo(json_object* artifact_obj, UpdateInfo& update_info) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Parsing artifact info"));
    
    // Get download URL - try both "_links" and "links" field names
    json_object* links_obj = nullptr;
    if (json_object_object_get_ex(artifact_obj, "_links", &links_obj)) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found _links field"));
    } else if (json_object_object_get_ex(artifact_obj, "links", &links_obj)) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found links field"));
    }
    
    if (links_obj) {
        json_object* download_obj;
        if (json_object_object_get_ex(links_obj, "download-http", &download_obj)) {
            json_object* href_obj;
            if (json_object_object_get_ex(download_obj, "href", &href_obj)) {
                update_info.download_url = json_object_get_string(href_obj);
                DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found download URL: "), DLT_STRING(update_info.download_url.c_str()));
            }
        }
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("No links field found in artifact"));
    }

    // Get description
    json_object* filename_obj;
    if (json_object_object_get_ex(artifact_obj, "filename", &filename_obj)) {
        update_info.description = json_object_get_string(filename_obj);
        DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Found filename: "), DLT_STRING(update_info.description.c_str()));
    }

    bool success = !update_info.download_url.empty();
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Artifact parsing "), DLT_STRING(success ? "successful" : "failed"));
    return success;
}

bool HawkbitClient::sendStartedFeedback(const std::string& execution_id) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Sending started feedback for execution: "), DLT_STRING(execution_id.c_str()));
    
    json_object* root = json_object_new_object();
    json_object* execution = json_object_new_object();
    json_object* result = json_object_new_object();
    
    json_object_object_add(result, "finished", json_object_new_string("proceeding"));
    json_object_object_add(result, "progress", json_object_new_int(0));
    json_object_object_add(result, "details", json_object_new_array());
    
    json_object_object_add(execution, "result", result);
    json_object_object_add(root, "id", json_object_new_string(execution_id.c_str()));
    json_object_object_add(root, "execution", execution);

    const char* json_string = json_object_to_json_string(root);
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Started feedback JSON: "), DLT_STRING(json_string));

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl_handle_, CURLOPT_URL, buildFeedbackUrl(execution_id).c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl_handle_);
    curl_slist_free_all(headers);
    json_object_put(root);

    if (res != CURLE_OK) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Started feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Started feedback sent successfully"));
        return true;
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Started feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool HawkbitClient::sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Sending progress feedback for execution: "), DLT_STRING(execution_id.c_str()), 
            DLT_STRING(" Progress: "), DLT_INT(progress), DLT_STRING("%"));
    
    json_object* root = json_object_new_object();
    json_object* execution = json_object_new_object();
    json_object* result = json_object_new_object();
    
    json_object_object_add(result, "finished", json_object_new_string("proceeding"));
    json_object_object_add(result, "progress", json_object_new_int(progress));
    
    json_object* details = json_object_new_array();
    if (!message.empty()) {
        json_object_array_add(details, json_object_new_string(message.c_str()));
    }
    json_object_object_add(result, "details", details);
    
    json_object_object_add(execution, "result", result);
    json_object_object_add(root, "id", json_object_new_string(execution_id.c_str()));
    json_object_object_add(root, "execution", execution);

    const char* json_string = json_object_to_json_string(root);
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Progress feedback JSON: "), DLT_STRING(json_string));

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl_handle_, CURLOPT_URL, buildFeedbackUrl(execution_id).c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl_handle_);
    curl_slist_free_all(headers);
    json_object_put(root);

    if (res != CURLE_OK) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Progress feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Progress feedback sent successfully: "), DLT_INT(progress), DLT_STRING("%"));
        return true;
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Progress feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool HawkbitClient::sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Sending finished feedback for execution: "), DLT_STRING(execution_id.c_str()),
            DLT_STRING(" Success: "), DLT_BOOL(success));
    
    json_object* root = json_object_new_object();
    json_object* execution = json_object_new_object();
    json_object* result = json_object_new_object();
    
    json_object_object_add(result, "finished", json_object_new_string(success ? "success" : "failure"));
    json_object_object_add(result, "progress", json_object_new_int(100));
    
    json_object* details = json_object_new_array();
    if (!message.empty()) {
        json_object_array_add(details, json_object_new_string(message.c_str()));
    }
    json_object_object_add(result, "details", details);
    
    json_object_object_add(execution, "result", result);
    json_object_object_add(root, "id", json_object_new_string(execution_id.c_str()));
    json_object_object_add(root, "execution", execution);

    const char* json_string = json_object_to_json_string(root);
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Finished feedback JSON: "), DLT_STRING(json_string));

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl_handle_, CURLOPT_URL, buildFeedbackUrl(execution_id).c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl_handle_);
    curl_slist_free_all(headers);
    json_object_put(root);

    if (res != CURLE_OK) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Finished feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Finished feedback sent successfully: "), DLT_STRING(success ? "success" : "failure"));
        return true;
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Finished feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool HawkbitClient::downloadBundle(const std::string& download_url, const std::string& local_path) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Downloading bundle from: "), DLT_STRING(download_url.c_str()));
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Bundle will be saved to: "), DLT_STRING(local_path.c_str()));
    
    if (!curl_handle_) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("CURL handle not initialized"));
        return false;
    }

    FILE* file = fopen(local_path.c_str(), "wb");
    if (!file) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to open file for writing: "), DLT_STRING(local_path.c_str()));
        return false;
    }

    curl_easy_setopt(curl_handle_, CURLOPT_URL, download_url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT_SECONDS);
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, FOLLOW_REDIRECTS ? 1L : 0L);

    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Starting download..."));
    CURLcode res = curl_easy_perform(curl_handle_);
    fclose(file);

    if (res != CURLE_OK) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Download failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Bundle downloaded successfully to: "), DLT_STRING(local_path.c_str()));
        return true;
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Download HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool HawkbitClient::sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message) {
    DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Sending feedback for execution: "), DLT_STRING(execution_id.c_str()));
    
    if (!curl_handle_) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("CURL handle not initialized"));
        return false;
    }

    std::string url = buildFeedbackUrl(execution_id);
    
    // Create JSON payload
    json_object* root = json_object_new_object();
    json_object* execution = json_object_new_object();
    json_object* result = json_object_new_object();
    
    json_object_object_add(result, "finished", json_object_new_string("success"));
    json_object_object_add(result, "progress", json_object_new_int(100));
    json_object_object_add(result, "details", json_object_new_array());
    
    json_object_object_add(execution, "result", result);
    json_object_object_add(root, "id", json_object_new_string(execution_id.c_str()));
    json_object_object_add(root, "execution", execution);
    
    if (!message.empty()) {
        json_object_object_add(result, "message", json_object_new_string(message.c_str()));
    }

    const char* json_string = json_object_to_json_string(root);
    DLT_LOG(hawkbitClientContext, DLT_LOG_DEBUG, DLT_STRING("Feedback JSON: "), DLT_STRING(json_string));

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_string);
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl_handle_);
    curl_slist_free_all(headers);
    json_object_put(root);

    if (res != CURLE_OK) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(hawkbitClientContext, DLT_LOG_INFO, DLT_STRING("Feedback sent successfully"));
        return true;
    } else {
        DLT_LOG(hawkbitClientContext, DLT_LOG_ERROR, DLT_STRING("Feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
} 