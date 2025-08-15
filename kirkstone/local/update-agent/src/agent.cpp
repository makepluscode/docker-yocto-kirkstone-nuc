
#include "agent.h"
#include "config.h"
#include <dlt/dlt.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

DLT_DECLARE_CONTEXT(dlt_context);

Agent::Agent(const std::string& server_url, const std::string& tenant, const std::string& device_id)
    : server_url_(server_url), tenant_(tenant), device_id_(device_id), curl_handle_(nullptr) {
    DLT_REGISTER_APP("UAGT", "Update Agent");
    DLT_REGISTER_CONTEXT(dlt_context, "AGENT", "Update Agent Logic");
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Initializing update agent"));
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Server URL: "), DLT_STRING(server_url_.c_str()));
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Tenant: "), DLT_STRING(tenant_.c_str()));
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Device ID: "), DLT_STRING(device_id_.c_str()));
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle_ = curl_easy_init();
    
    if (curl_handle_) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("CURL handle initialized successfully"));
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to initialize CURL handle"));
    }
}

Agent::~Agent() {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Cleaning up update agent"));
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("CURL handle cleaned up"));
    }
    curl_global_cleanup();
    DLT_UNREGISTER_CONTEXT(dlt_context);
    DLT_UNREGISTER_APP();
}

size_t Agent::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    if (!userp) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("writeCallback: userp is null"));
        return 0;
    }
    
    if (!contents) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("writeCallback: contents is null"));
        return 0;
    }
    
    size_t total_size = size * nmemb;
    
    try {
        userp->append((char*)contents, total_size);
        return total_size;
    } catch (const std::exception& e) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Exception in writeCallback: "), DLT_STRING(e.what()));
        return 0;
    } catch (...) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Unknown exception in writeCallback"));
        return 0;
    }
}

size_t Agent::writeFileCallback(void* contents, size_t size, size_t nmemb, FILE* file) {
    if (!contents || !file || size == 0 || nmemb == 0) {
        return 0;
    }
    return fwrite(contents, size, nmemb, file);
}

std::string Agent::buildPollUrl() const {
    std::string url = server_url_ + "/" + tenant_ + "/controller/v1/" + device_id_;
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Built poll URL: "), DLT_STRING(url.c_str()));
    return url;
}

std::string Agent::buildFeedbackUrl(const std::string& execution_id) const {
    std::string url = server_url_ + "/" + tenant_ + "/controller/v1/" + device_id_ + "/deploymentBase/" + execution_id + "/feedback";
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Built feedback URL: "), DLT_STRING(url.c_str()));
    return url;
}

void Agent::setupDownloadCurlOptions() {
    if (!curl_handle_) return;
    
    // Based on rauc-hawkbit-updater reference implementation
    
    // Basic options
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT_SECONDS);
    
    // Connection timeout
    curl_easy_setopt(curl_handle_, CURLOPT_CONNECTTIMEOUT, 30L);
    
    // Low speed limit (abort if speed drops below 1KB/s for 60 seconds)
    curl_easy_setopt(curl_handle_, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl_handle_, CURLOPT_LOW_SPEED_TIME, 60L);
    
    // SSL options
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, ENABLE_SSL_VERIFICATION ? 1L : 0L);
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, ENABLE_SSL_VERIFICATION ? 2L : 0L);
    
    // User agent
    curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, "rauc-hawkbit-cpp/1.0");
    
    // Accept ranges for resumable downloads
    curl_easy_setopt(curl_handle_, CURLOPT_RANGE, NULL); // Clear any previous range
    
    // Disable progress meter to avoid interference with DLT logging
    curl_easy_setopt(curl_handle_, CURLOPT_NOPROGRESS, 1L);
    
    // Enable TCP keep-alive
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPINTVL, 60L);
    
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("CURL download options configured"));
}

bool Agent::pollForUpdates(std::string& response) {
    if (!curl_handle_) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("CURL handle not initialized"));
        return false;
    }

    // Reset CURL handle to clean state
    curl_easy_reset(curl_handle_);
    
    response.clear();
    std::string url = buildPollUrl();

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Polling for updates from: "), DLT_STRING(url.c_str()));

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SECONDS);
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, FOLLOW_REDIRECTS ? 1L : 0L);

    CURLcode res = curl_easy_perform(curl_handle_);
    if (res != CURLE_OK) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("curl_easy_perform() failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Poll response HTTP code: "), DLT_INT(http_code));

    if (http_code == 200) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Poll successful, response length: "), DLT_UINT(response.length()));
        DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Poll response: "), DLT_STRING(response.c_str()));
        return true;
    } else if (http_code == 204) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("No updates available (HTTP 204)"));
        return true;
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool Agent::parseUpdateResponse(const std::string& response, UpdateInfo& update_info) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Parsing update response"));
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Response length: "), DLT_UINT(response.length()));
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Response content: "), DLT_STRING(response.c_str()));
    
    update_info.is_available = false;
    
    if (response.empty()) {
        DLT_LOG(dlt_context, DLT_LOG_WARN, DLT_STRING("Empty response received"));
        return false;
    }

    json_object* root = json_tokener_parse(response.c_str());
    if (!root) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to parse JSON response"));
        return false;
    }

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("JSON parsed successfully"));

    // Check if there's a deployment
    json_object* deployment_obj;
    if (json_object_object_get_ex(root, "deployment", &deployment_obj)) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Deployment object found in response"));
        if (parseDeploymentInfo(deployment_obj, update_info)) {
            update_info.is_available = true;
            DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Update info parsed successfully"));
            DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
            DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Version: "), DLT_STRING(update_info.version.c_str()));
            DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Download URL: "), DLT_STRING(update_info.download_url.c_str()));
        } else {
            DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to parse deployment info"));
        }
    } else {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("No deployment object in response"));
    }

    json_object_put(root);
    return update_info.is_available;
}

bool Agent::parseDeploymentInfo(json_object* deployment_obj, UpdateInfo& update_info) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Parsing deployment info"));
    
    // Get execution ID
    json_object* execution_id_obj;
    if (json_object_object_get_ex(deployment_obj, "id", &execution_id_obj)) {
        update_info.execution_id = json_object_get_string(execution_id_obj);
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found execution ID: "), DLT_STRING(update_info.execution_id.c_str()));
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("No execution ID found in deployment"));
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
                DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found "), DLT_INT(array_len), DLT_STRING(" chunks"));
                if (array_len > 0) {
                    json_object* first_chunk = json_object_array_get_idx(version_obj, 0);
                    json_object* version_info_obj;
                    if (json_object_object_get_ex(first_chunk, "version", &version_info_obj)) {
                        update_info.version = json_object_get_string(version_info_obj);
                        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found version: "), DLT_STRING(update_info.version.c_str()));
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
            DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found "), DLT_INT(array_len), DLT_STRING(" artifacts"));
            if (array_len > 0) {
                json_object* first_artifact = json_object_array_get_idx(artifacts_obj, 0);
                if (parseArtifactInfo(first_artifact, update_info)) {
                    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Artifact info parsed successfully"));
                    return true;
                }
            }
        }
    }

    DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to parse deployment info completely"));
    return false;
}

bool Agent::parseArtifactInfo(json_object* artifact_obj, UpdateInfo& update_info) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Parsing artifact info"));
    
    // Get download URL - try both "_links" and "links" field names
    json_object* links_obj = nullptr;
    if (json_object_object_get_ex(artifact_obj, "_links", &links_obj)) {
        DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Found _links field"));
    } else if (json_object_object_get_ex(artifact_obj, "links", &links_obj)) {
        DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Found links field"));
    }
    
    if (links_obj) {
        json_object* download_obj;
        if (json_object_object_get_ex(links_obj, "download-http", &download_obj)) {
            json_object* href_obj;
            if (json_object_object_get_ex(download_obj, "href", &href_obj)) {
                update_info.download_url = json_object_get_string(href_obj);
                DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found download URL: "), DLT_STRING(update_info.download_url.c_str()));
            }
        }
    } else {
        DLT_LOG(dlt_context, DLT_LOG_WARN, DLT_STRING("No links field found in artifact"));
    }

    // Get filename and description
    json_object* filename_obj;
    if (json_object_object_get_ex(artifact_obj, "filename", &filename_obj)) {
        update_info.filename = json_object_get_string(filename_obj);
        update_info.description = update_info.filename; // Use filename as description
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found filename: "), DLT_STRING(update_info.filename.c_str()));
    }
    
    // Get file size
    json_object* size_obj;
    if (json_object_object_get_ex(artifact_obj, "size", &size_obj)) {
        update_info.expected_size = json_object_get_int64(size_obj);
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Found expected file size: "), DLT_INT64(update_info.expected_size), DLT_STRING(" bytes"));
    }
    
    // Get hash information (hashes object contains MD5, SHA1, SHA256)
    json_object* hashes_obj;
    if (json_object_object_get_ex(artifact_obj, "hashes", &hashes_obj)) {
        DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Found hashes object"));
        
        json_object* md5_obj;
        if (json_object_object_get_ex(hashes_obj, "md5", &md5_obj)) {
            update_info.md5_hash = json_object_get_string(md5_obj);
            DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Found MD5 hash: "), DLT_STRING(update_info.md5_hash.c_str()));
        }
        
        json_object* sha1_obj;
        if (json_object_object_get_ex(hashes_obj, "sha1", &sha1_obj)) {
            update_info.sha1_hash = json_object_get_string(sha1_obj);
            DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Found SHA1 hash: "), DLT_STRING(update_info.sha1_hash.c_str()));
        }
        
        json_object* sha256_obj;
        if (json_object_object_get_ex(hashes_obj, "sha256", &sha256_obj)) {
            update_info.sha256_hash = json_object_get_string(sha256_obj);
            DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Found SHA256 hash: "), DLT_STRING(update_info.sha256_hash.c_str()));
        }
    }

    bool success = !update_info.download_url.empty();
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Artifact parsing "), DLT_STRING(success ? "successful" : "failed"));
    return success;
}

bool Agent::sendStartedFeedback(const std::string& execution_id) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Sending started feedback for execution: "), DLT_STRING(execution_id.c_str()));
    
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
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Started feedback JSON: "), DLT_STRING(json_string));

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
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Started feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Started feedback sent successfully"));
        return true;
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Started feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool Agent::sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Sending progress feedback for execution: "), DLT_STRING(execution_id.c_str()), DLT_STRING(" Progress: "), DLT_INT(progress), DLT_STRING("%"));
    
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
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Progress feedback JSON: "), DLT_STRING(json_string));

    // Copy the JSON string before freeing the object
    std::string json_copy(json_string);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl_handle_, CURLOPT_URL, buildFeedbackUrl(execution_id).c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, json_copy.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl_handle_);
    curl_slist_free_all(headers);
    json_object_put(root);

    if (res != CURLE_OK) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Progress feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Progress feedback sent successfully: "), DLT_INT(progress), DLT_STRING("%"));
        return true;
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Progress feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool Agent::sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Sending finished feedback for execution: "), DLT_STRING(execution_id.c_str()), DLT_STRING(" Success: "), DLT_BOOL(success));
    
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
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Finished feedback JSON: "), DLT_STRING(json_string));

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
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Finished feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Finished feedback sent successfully: "), DLT_STRING(success ? "success" : "failure"));
        return true;
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Finished feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
}

bool Agent::downloadBundle(const std::string& download_url, const std::string& local_path) {
    return downloadBundle(download_url, local_path, 0); // Call with no expected size check
}

bool Agent::downloadBundle(const std::string& download_url, const std::string& local_path, long expected_size) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("=== Starting bundle download ==="));
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Download URL: "), DLT_STRING(download_url.c_str()));
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Local path: "), DLT_STRING(local_path.c_str()));

    // Validate inputs
    if (download_url.empty() || local_path.empty()) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Invalid download parameters"));
        return false;
    }

    if (!curl_handle_) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("CURL handle not initialized"));
        return false;
    }

    // Remove existing file if present
    if (access(local_path.c_str(), F_OK) == 0) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Removing existing file"));
        if (remove(local_path.c_str()) != 0) {
            DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to remove existing file"));
        }
    }

    // Open file for writing with explicit error handling
    FILE* file = fopen(local_path.c_str(), "wb");
    if (!file) {
        int err = errno;
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to open file for writing"));
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Error: "), DLT_STRING(strerror(err)));
        return false;
    }

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("File opened successfully for writing"));

    // Reset CURL handle to clean state
    curl_easy_reset(curl_handle_);
    
    // Configure CURL options step by step
    curl_easy_setopt(curl_handle_, CURLOPT_URL, download_url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 300L); // 5 minute timeout
    curl_easy_setopt(curl_handle_, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, "rauc-hawkbit-cpp/1.0");
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 0L); // Disable for debugging
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle_, CURLOPT_NOPROGRESS, 1L);

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("CURL configured, starting download..."));

    // Perform download
    auto start_time = std::chrono::steady_clock::now();
    CURLcode res = curl_easy_perform(curl_handle_);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Always close file first
    if (file) {
        fflush(file);
        fclose(file);
        file = nullptr;
    }

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Download completed in "), DLT_INT(duration.count()), DLT_STRING(" ms"));

    // Check CURL result
    if (res != CURLE_OK) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("CURL error: "), DLT_STRING(curl_easy_strerror(res)));
        remove(local_path.c_str()); // Clean up partial file
        return false;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("HTTP response code: "), DLT_INT(http_code));

    if (http_code != 200) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("HTTP error: "), DLT_INT(http_code));
        remove(local_path.c_str()); // Clean up partial file
        return false;
    }

    // Verify file was written and check size
    struct stat file_info;
    if (stat(local_path.c_str(), &file_info) == 0) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Downloaded file size: "), DLT_INT64(file_info.st_size), DLT_STRING(" bytes"));
        
        if (file_info.st_size == 0) {
            DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Downloaded file is empty"));
            remove(local_path.c_str());
            return false;
        }
        
        // Verify expected file size if provided
        if (expected_size > 0) {
            if (file_info.st_size != expected_size) {
                DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("File size mismatch! Expected: "), DLT_INT64(expected_size), DLT_STRING(" bytes, got: "), DLT_INT64(file_info.st_size), DLT_STRING(" bytes"));
                remove(local_path.c_str());
                return false;
            } else {
                DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("File size verification passed"));
            }
        }
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Failed to stat downloaded file"));
        return false;
    }

    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("=== Bundle download successful ==="));
    return true;
}

bool Agent::sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message) {
    DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Sending feedback for execution: "), DLT_STRING(execution_id.c_str()));
    
    if (!curl_handle_) {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("CURL handle not initialized"));
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
    DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_STRING("Feedback JSON: "), DLT_STRING(json_string));

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
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Feedback send failed: "), DLT_STRING(curl_easy_strerror(res)));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_STRING("Feedback sent successfully"));
        return true;
    } else {
        DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_STRING("Feedback HTTP error: "), DLT_INT(http_code));
        return false;
    }
} 