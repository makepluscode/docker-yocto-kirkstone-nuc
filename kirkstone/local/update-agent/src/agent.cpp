
#include "agent.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

Agent::Agent(const std::string& server_url, const std::string& tenant, const std::string& device_id)
    : server_url_(server_url), tenant_(tenant), device_id_(device_id), curl_handle_(nullptr) {
    std::cout << "Initializing update agent" << std::endl;
    std::cout << "Server URL: " << server_url_ << std::endl;
    std::cout << "Tenant: " << tenant_ << std::endl;
    std::cout << "Device ID: " << device_id_ << std::endl;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle_ = curl_easy_init();
    
    if (curl_handle_) {
        std::cout << "CURL handle initialized successfully" << std::endl;
    } else {
        std::cout << "Failed to initialize CURL handle" << std::endl;
    }
}

Agent::~Agent() {
    std::cout << "Cleaning up update agent" << std::endl;
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
        std::cout << "CURL handle cleaned up" << std::endl;
    }
    curl_global_cleanup();
}

size_t Agent::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    if (!userp) {
        std::cout << "writeCallback: userp is null" << std::endl;
        return 0;
    }
    
    if (!contents) {
        std::cout << "writeCallback: contents is null" << std::endl;
        return 0;
    }
    
    size_t total_size = size * nmemb;
    
    try {
        userp->append((char*)contents, total_size);
        return total_size;
    } catch (const std::exception& e) {
        std::cout << "Exception in writeCallback: " << e.what() << std::endl;
        return 0;
    } catch (...) {
        std::cout << "Unknown exception in writeCallback" << std::endl;
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
    std::cout << "Built poll URL: " << url << std::endl;
    return url;
}

std::string Agent::buildFeedbackUrl(const std::string& execution_id) const {
    std::string url = server_url_ + "/" + tenant_ + "/controller/v1/" + device_id_ + "/deploymentBase/" + execution_id + "/feedback";
    std::cout << "Built feedback URL: " << url << std::endl;
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
    
    std::cout << "CURL download options configured" << std::endl;
}

bool Agent::pollForUpdates(std::string& response) {
    if (!curl_handle_) {
        std::cout << "CURL handle not initialized" << std::endl;
        return false;
    }

    // Reset CURL handle to clean state
    curl_easy_reset(curl_handle_);
    
    response.clear();
    std::string url = buildPollUrl();

    std::cout << "Polling for updates from: " << url << std::endl;

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SECONDS);
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, FOLLOW_REDIRECTS ? 1L : 0L);

    CURLcode res = curl_easy_perform(curl_handle_);
    if (res != CURLE_OK) {
        std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    std::cout << "Poll response HTTP code: " << http_code << std::endl;

    if (http_code == 200) {
        std::cout << "Poll successful, response length: " << response.length() << std::endl;
        std::cout << "Poll response: " << response << std::endl;
        return true;
    } else if (http_code == 204) {
        std::cout << "No updates available (HTTP 204)" << std::endl;
        return true;
    } else {
        std::cout << "HTTP error: " << http_code << std::endl;
        return false;
    }
}

bool Agent::parseUpdateResponse(const std::string& response, UpdateInfo& update_info) {
    std::cout << "Parsing update response" << std::endl;
    std::cout << "Response length: " << response.length() << std::endl;
    std::cout << "Response content: " << response << std::endl;
    
    update_info.is_available = false;
    
    if (response.empty()) {
        std::cout << "Empty response received" << std::endl;
        return false;
    }

    json_object* root = json_tokener_parse(response.c_str());
    if (!root) {
        std::cout << "Failed to parse JSON response" << std::endl;
        return false;
    }

    std::cout << "JSON parsed successfully" << std::endl;

    // Check if there's a deployment
    json_object* deployment_obj;
    if (json_object_object_get_ex(root, "deployment", &deployment_obj)) {
        std::cout << "Deployment object found in response" << std::endl;
        if (parseDeploymentInfo(deployment_obj, update_info)) {
            update_info.is_available = true;
            std::cout << "Update info parsed successfully" << std::endl;
            std::cout << "Execution ID: " << update_info.execution_id << std::endl;
            std::cout << "Version: " << update_info.version << std::endl;
            std::cout << "Download URL: " << update_info.download_url << std::endl;
        } else {
            std::cout << "Failed to parse deployment info" << std::endl;
        }
    } else {
        std::cout << "No deployment object in response" << std::endl;
    }

    json_object_put(root);
    return update_info.is_available;
}

bool Agent::parseDeploymentInfo(json_object* deployment_obj, UpdateInfo& update_info) {
    std::cout << "Parsing deployment info" << std::endl;
    
    // Get execution ID
    json_object* execution_id_obj;
    if (json_object_object_get_ex(deployment_obj, "id", &execution_id_obj)) {
        update_info.execution_id = json_object_get_string(execution_id_obj);
        std::cout << "Found execution ID: " << update_info.execution_id << std::endl;
    } else {
        std::cout << "No execution ID found in deployment" << std::endl;
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
                std::cout << "Found " << array_len << " chunks" << std::endl;
                if (array_len > 0) {
                    json_object* first_chunk = json_object_array_get_idx(version_obj, 0);
                    json_object* version_info_obj;
                    if (json_object_object_get_ex(first_chunk, "version", &version_info_obj)) {
                        update_info.version = json_object_get_string(version_info_obj);
                        std::cout << "Found version: " << update_info.version << std::endl;
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
            std::cout << "Found " << array_len << " artifacts" << std::endl;
            if (array_len > 0) {
                json_object* first_artifact = json_object_array_get_idx(artifacts_obj, 0);
                if (parseArtifactInfo(first_artifact, update_info)) {
                    std::cout << "Artifact info parsed successfully" << std::endl;
                    return true;
                }
            }
        }
    }

    std::cout << "Failed to parse deployment info completely" << std::endl;
    return false;
}

bool Agent::parseArtifactInfo(json_object* artifact_obj, UpdateInfo& update_info) {
    std::cout << "Parsing artifact info" << std::endl;
    
    // Get download URL - try both "_links" and "links" field names
    json_object* links_obj = nullptr;
    if (json_object_object_get_ex(artifact_obj, "_links", &links_obj)) {
        std::cout << "Found _links field" << std::endl;
    } else if (json_object_object_get_ex(artifact_obj, "links", &links_obj)) {
        std::cout << "Found links field" << std::endl;
    }
    
    if (links_obj) {
        json_object* download_obj;
        if (json_object_object_get_ex(links_obj, "download-http", &download_obj)) {
            json_object* href_obj;
            if (json_object_object_get_ex(download_obj, "href", &href_obj)) {
                update_info.download_url = json_object_get_string(href_obj);
                std::cout << "Found download URL: " << update_info.download_url << std::endl;
            }
        }
    } else {
        std::cout << "No links field found in artifact" << std::endl;
    }

    // Get filename and description
    json_object* filename_obj;
    if (json_object_object_get_ex(artifact_obj, "filename", &filename_obj)) {
        update_info.filename = json_object_get_string(filename_obj);
        update_info.description = update_info.filename; // Use filename as description
        std::cout << "Found filename: " << update_info.filename << std::endl;
    }
    
    // Get file size
    json_object* size_obj;
    if (json_object_object_get_ex(artifact_obj, "size", &size_obj)) {
        update_info.expected_size = json_object_get_int64(size_obj);
        std::cout << "Found expected file size: " << update_info.expected_size << " bytes" << std::endl;
    }
    
    // Get hash information (hashes object contains MD5, SHA1, SHA256)
    json_object* hashes_obj;
    if (json_object_object_get_ex(artifact_obj, "hashes", &hashes_obj)) {
        std::cout << "Found hashes object" << std::endl;
        
        json_object* md5_obj;
        if (json_object_object_get_ex(hashes_obj, "md5", &md5_obj)) {
            update_info.md5_hash = json_object_get_string(md5_obj);
            std::cout << "Found MD5 hash: " << update_info.md5_hash << std::endl;
        }
        
        json_object* sha1_obj;
        if (json_object_object_get_ex(hashes_obj, "sha1", &sha1_obj)) {
            update_info.sha1_hash = json_object_get_string(sha1_obj);
            std::cout << "Found SHA1 hash: " << update_info.sha1_hash << std::endl;
        }
        
        json_object* sha256_obj;
        if (json_object_object_get_ex(hashes_obj, "sha256", &sha256_obj)) {
            update_info.sha256_hash = json_object_get_string(sha256_obj);
            std::cout << "Found SHA256 hash: " << update_info.sha256_hash << std::endl;
        }
    }

    bool success = !update_info.download_url.empty();
    std::cout << "Artifact parsing " << (success ? "successful" : "failed") << std::endl;
    return success;
}

bool Agent::sendStartedFeedback(const std::string& execution_id) {
    std::cout << "Sending started feedback for execution: " << execution_id << std::endl;
    
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
    std::cout << "Started feedback JSON: " << json_string << std::endl;

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
        std::cout << "Started feedback send failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Started feedback sent successfully" << std::endl;
        return true;
    } else {
        std::cout << "Started feedback HTTP error: " << http_code << std::endl;
        return false;
    }
}

bool Agent::sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message) {
    std::cout << "Sending progress feedback for execution: " << execution_id << " Progress: " << progress << "%" << std::endl;
    
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
    std::cout << "Progress feedback JSON: " << json_string << std::endl;

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
        std::cout << "Progress feedback send failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Progress feedback sent successfully: " << progress << "%" << std::endl;
        return true;
    } else {
        std::cout << "Progress feedback HTTP error: " << http_code << std::endl;
        return false;
    }
}

bool Agent::sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message) {
    std::cout << "Sending finished feedback for execution: " << execution_id << " Success: " << (success ? "true" : "false") << std::endl;
    
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
    std::cout << "Finished feedback JSON: " << json_string << std::endl;

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
        std::cout << "Finished feedback send failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Finished feedback sent successfully: " << (success ? "success" : "failure") << std::endl;
        return true;
    } else {
        std::cout << "Finished feedback HTTP error: " << http_code << std::endl;
        return false;
    }
}

bool Agent::downloadBundle(const std::string& download_url, const std::string& local_path) {
    return downloadBundle(download_url, local_path, 0); // Call with no expected size check
}

bool Agent::downloadBundle(const std::string& download_url, const std::string& local_path, long expected_size) {
    std::cout << "=== Starting bundle download ===" << std::endl;
    std::cout << "Download URL: " << download_url << std::endl;
    std::cout << "Local path: " << local_path << std::endl;

    // Validate inputs
    if (download_url.empty() || local_path.empty()) {
        std::cout << "Invalid download parameters" << std::endl;
        return false;
    }

    if (!curl_handle_) {
        std::cout << "CURL handle not initialized" << std::endl;
        return false;
    }

    // Remove existing file if present
    if (access(local_path.c_str(), F_OK) == 0) {
        std::cout << "Removing existing file" << std::endl;
        if (remove(local_path.c_str()) != 0) {
            std::cout << "Failed to remove existing file" << std::endl;
        }
    }

    // Open file for writing with explicit error handling
    FILE* file = fopen(local_path.c_str(), "wb");
    if (!file) {
        int err = errno;
        std::cout << "Failed to open file for writing" << std::endl;
        std::cout << "Error: " << strerror(err) << std::endl;
        return false;
    }

    std::cout << "File opened successfully for writing" << std::endl;

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

    std::cout << "CURL configured, starting download..." << std::endl;

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

    std::cout << "Download completed in " << duration.count() << " ms" << std::endl;

    // Check CURL result
    if (res != CURLE_OK) {
        std::cout << "CURL error: " << curl_easy_strerror(res) << std::endl;
        remove(local_path.c_str()); // Clean up partial file
        return false;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
    std::cout << "HTTP response code: " << http_code << std::endl;

    if (http_code != 200) {
        std::cout << "HTTP error: " << http_code << std::endl;
        remove(local_path.c_str()); // Clean up partial file
        return false;
    }

    // Verify file was written and check size
    struct stat file_info;
    if (stat(local_path.c_str(), &file_info) == 0) {
        std::cout << "Downloaded file size: " << file_info.st_size << " bytes" << std::endl;
        
        if (file_info.st_size == 0) {
            std::cout << "Downloaded file is empty" << std::endl;
            remove(local_path.c_str());
            return false;
        }
        
        // Verify expected file size if provided
        if (expected_size > 0) {
            if (file_info.st_size != expected_size) {
                std::cout << "File size mismatch! Expected: " << expected_size << " bytes, got: " << file_info.st_size << " bytes" << std::endl;
                remove(local_path.c_str());
                return false;
            } else {
                std::cout << "File size verification passed" << std::endl;
            }
        }
    } else {
        std::cout << "Failed to stat downloaded file" << std::endl;
        return false;
    }

    std::cout << "=== Bundle download successful ===" << std::endl;
    return true;
}

bool Agent::sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message) {
    std::cout << "Sending feedback for execution: " << execution_id << std::endl;
    
    if (!curl_handle_) {
        std::cout << "CURL handle not initialized" << std::endl;
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
    std::cout << "Feedback JSON: " << json_string << std::endl;

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
        std::cout << "Feedback send failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Feedback sent successfully" << std::endl;
        return true;
    } else {
        std::cout << "Feedback HTTP error: " << http_code << std::endl;
        return false;
    }
} 