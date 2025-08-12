#ifndef RAUC_CLIENT_H
#define RAUC_CLIENT_H

#include <string>
#include <dbus/dbus.h>
#include <functional>

class RaucClient {
public:
    RaucClient();
    ~RaucClient();

    bool connect();
    void disconnect();
    bool isConnected() const;

    // RAUC operations via DBus
    bool installBundle(const std::string& bundle_path);
    bool getStatus(std::string& status);
    bool getBootSlot(std::string& boot_slot);
    bool markGood();
    bool markBad();
    bool getBundleInfo(const std::string& bundle_path, std::string& info);

    // Signal handlers
    void setProgressCallback(std::function<void(int)> callback);
    void setCompletedCallback(std::function<void(bool, const std::string&)> callback);

private:
    DBusConnection* connection_;
    bool connected_;
    std::function<void(int)> progress_callback_;
    std::function<void(bool, const std::string&)> completed_callback_;

    bool sendMethodCall(const std::string& method, const std::string& interface = "de.pengutronix.rauc.Installer");
    bool sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface = "de.pengutronix.rauc.Installer");
    static DBusHandlerResult messageHandler(DBusConnection* connection, DBusMessage* message, void* user_data);
    void handleSignal(DBusMessage* message);
};

#endif // RAUC_CLIENT_H 