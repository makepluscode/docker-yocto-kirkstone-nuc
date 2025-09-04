#ifndef UPDATE_CLIENT_H
#define UPDATE_CLIENT_H

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

class UpdateClient {
public:
    UpdateClient(const std::string& server_url, const std::string& tenant, const std::string& device_id);
    ~UpdateClient();

    bool pollForUpdates(std::string& response);
    bool downloadBundle(const std::string& download_url, const std::string& local_path);
    bool downloadBundle(const std::string& download_url, const std::string& local_path, long expected_size);
    bool sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message = "");

    bool parseUpdateResponse(const std::string& response, UpdateInfo& update_info);
    bool sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message = "");
    bool sendStartedFeedback(const std::string& execution_id);
    bool sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message = "");

private:
    std::string server_url_;
    std::string tenant_;
    std::string device_id_;
    CURL* curl_handle_;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static size_t writeFileCallback(void* contents, size_t size, size_t nmemb, FILE* file);
    std::string buildPollUrl() const;
    std::string buildFeedbackUrl(const std::string& execution_id) const;
    void setupDownloadCurlOptions();

    bool parseDeploymentInfo(json_object* deployment_obj, UpdateInfo& update_info);
    bool parseArtifactInfo(json_object* artifact_obj, UpdateInfo& update_info);
};

#endif // UPDATE_CLIENT_H 