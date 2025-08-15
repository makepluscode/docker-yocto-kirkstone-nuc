#ifndef HAWKBIT_CLIENT_H
#define HAWKBIT_CLIENT_H

#include <string>
#include <memory>
#include <vector>
#include <curl/curl.h>
#include <json-c/json.h>

// Update information structure
struct UpdateInfo {
    std::string execution_id;
    std::string download_url;
    std::string version;
    std::string description;
    std::string filename;
    long expected_size;
    std::string md5_hash;
    std::string sha1_hash;
    std::string sha256_hash;
    bool is_available;
    
    UpdateInfo() : expected_size(0), is_available(false) {}
};

class HawkbitClient {
public:
    HawkbitClient(const std::string& server_url, const std::string& tenant, const std::string& controller_id);
    ~HawkbitClient();

    bool pollForUpdates(std::string& response);
    bool downloadBundle(const std::string& download_url, const std::string& local_path);
    bool downloadBundle(const std::string& download_url, const std::string& local_path, long expected_size);
    bool sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message = "");
    
    // New methods for update handling
    bool parseUpdateResponse(const std::string& response, UpdateInfo& update_info);
    bool sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message = "");
    bool sendStartedFeedback(const std::string& execution_id);
    bool sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message = "");

private:
    std::string server_url_;
    std::string tenant_;
    std::string controller_id_;
    CURL* curl_handle_;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static size_t writeFileCallback(void* contents, size_t size, size_t nmemb, FILE* file);
    std::string buildPollUrl() const;
    std::string buildFeedbackUrl(const std::string& execution_id) const;
    void setupDownloadCurlOptions();
    
    // Helper methods for JSON parsing
    bool parseDeploymentInfo(json_object* deployment_obj, UpdateInfo& update_info);
    bool parseArtifactInfo(json_object* artifact_obj, UpdateInfo& update_info);
};

#endif // HAWKBIT_CLIENT_H 