#ifndef HAWKBIT_CLIENT_H
#define HAWKBIT_CLIENT_H

#include <string>
#include <memory>
#include <curl/curl.h>
#include <json-c/json.h>

class HawkbitClient {
public:
    HawkbitClient(const std::string& server_url, const std::string& tenant, const std::string& controller_id);
    ~HawkbitClient();

    bool pollForUpdates();
    bool downloadBundle(const std::string& download_url, const std::string& local_path);
    bool sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message = "");

private:
    std::string server_url_;
    std::string tenant_;
    std::string controller_id_;
    CURL* curl_handle_;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static size_t writeFileCallback(void* contents, size_t size, size_t nmemb, FILE* file);
    std::string buildPollUrl() const;
    std::string buildFeedbackUrl(const std::string& execution_id) const;
};

#endif // HAWKBIT_CLIENT_H 