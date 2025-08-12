#include "hawkbit_client.h"
#include <iostream>
#include <fstream>
#include <sstream>

HawkbitClient::HawkbitClient(const std::string& server_url, const std::string& tenant, const std::string& controller_id)
    : server_url_(server_url), tenant_(tenant), controller_id_(controller_id), curl_handle_(nullptr) {
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle_ = curl_easy_init();
}

HawkbitClient::~HawkbitClient() {
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
    }
    curl_global_cleanup();
}

size_t HawkbitClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t HawkbitClient::writeFileCallback(void* contents, size_t size, size_t nmemb, FILE* file) {
    return fwrite(contents, size, nmemb, file);
}

std::string HawkbitClient::buildPollUrl() const {
    return server_url_ + "/" + tenant_ + "/controller/v1/" + controller_id_;
}

std::string HawkbitClient::buildFeedbackUrl(const std::string& execution_id) const {
    return server_url_ + "/" + tenant_ + "/controller/v1/" + controller_id_ + "/deploymentBase/" + execution_id + "/feedback";
}

bool HawkbitClient::pollForUpdates() {
    if (!curl_handle_) {
        std::cerr << "CURL handle not initialized" << std::endl;
        return false;
    }

    std::string response;
    std::string url = buildPollUrl();

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl_handle_);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Poll response: " << response << std::endl;
        return true;
    } else if (http_code == 204) {
        std::cout << "No updates available" << std::endl;
        return true;
    } else {
        std::cerr << "HTTP error: " << http_code << std::endl;
        return false;
    }
}

bool HawkbitClient::downloadBundle(const std::string& download_url, const std::string& local_path) {
    if (!curl_handle_) {
        std::cerr << "CURL handle not initialized" << std::endl;
        return false;
    }

    FILE* file = fopen(local_path.c_str(), "wb");
    if (!file) {
        std::cerr << "Failed to open file for writing: " << local_path << std::endl;
        return false;
    }

    curl_easy_setopt(curl_handle_, CURLOPT_URL, download_url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 300L); // 5 minutes timeout for downloads
    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl_handle_);
    fclose(file);

    if (res != CURLE_OK) {
        std::cerr << "Download failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Bundle downloaded successfully to: " << local_path << std::endl;
        return true;
    } else {
        std::cerr << "Download HTTP error: " << http_code << std::endl;
        return false;
    }
}

bool HawkbitClient::sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message) {
    if (!curl_handle_) {
        std::cerr << "CURL handle not initialized" << std::endl;
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
        std::cerr << "Feedback send failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 200) {
        std::cout << "Feedback sent successfully" << std::endl;
        return true;
    } else {
        std::cerr << "Feedback HTTP error: " << http_code << std::endl;
        return false;
    }
} 