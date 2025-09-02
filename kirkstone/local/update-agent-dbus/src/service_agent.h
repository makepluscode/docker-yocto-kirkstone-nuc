#ifndef SERVICE_AGENT_H
#define SERVICE_AGENT_H

#include <string>
#include <dbus/dbus.h>
#include <functional>

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
    DBusConnection* connection_;
    bool connected_;
    std::function<void(int)> progress_callback_;
    std::function<void(bool, const std::string&)> completed_callback_;

    bool sendMethodCall(const std::string& method, const std::string& interface);
    bool sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface);
    static DBusHandlerResult messageHandler(DBusConnection* connection, DBusMessage* message, void* user_data);
    void handleSignal(DBusMessage* message);
};

#endif // SERVICE_AGENT_H
