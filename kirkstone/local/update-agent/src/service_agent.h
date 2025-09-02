#ifndef SERVICE_AGENT_H
#define SERVICE_AGENT_H

#include <string>
#include <functional>
#include <memory>
#include "update_client.h"

class ServiceAgent {
public:
    ServiceAgent();
    ~ServiceAgent();

    bool connect();
    void disconnect();
    bool isConnected() const;
    bool checkService();

    bool installBundle(const std::string& bundle_path);
    bool installBundleAsync(const std::string& bundle_path);
    bool getStatus(std::string& status);
    bool getBootSlot(std::string& boot_slot);
    bool markGood();
    bool markBad();
    bool getBundleInfo(const std::string& bundle_path, std::string& info);

    void setProgressCallback(std::function<void(int)> callback);
    void setCompletedCallback(std::function<void(bool, const std::string&)> callback);
    void processMessages();

private:
    std::unique_ptr<UpdateClient> update_client_;
    bool connected_;
    std::function<void(int)> progress_callback_;
    std::function<void(bool, const std::string&)> completed_callback_;

    // Internal callback handlers for update-library
    void onProgressCallback(const ProgressInfo& progress);
    void onCompletedCallback(InstallResult result, const std::string& message);
};

#endif // SERVICE_AGENT_H
