#ifndef PACKAGE_INSTALLER_H
#define PACKAGE_INSTALLER_H

#include <string>
#include <functional>
#include <memory>
#include "update_client.h"

class PackageInstaller {
public:
    PackageInstaller();
    ~PackageInstaller();

    bool connect();
    void disconnect();
    bool isConnected() const;
    bool checkService();

    bool installPackage(const std::string& package_path);
    bool installPackageAsync(const std::string& package_path);
    bool getStatus(std::string& status);
    bool getBootSlot(std::string& boot_slot);
    bool markGood();
    bool markBad();
    bool getPackageInfo(const std::string& package_path, std::string& info);

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

#endif // PACKAGE_INSTALLER_H
