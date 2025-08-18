#pragma once

#include <dbus/dbus.h>
#include <string>
#include <functional>
#include <memory>

/**
 * @brief Update Service D-Bus Broker
 *
 * This class implements a D-Bus broker service that acts as an abstraction layer
 * between update-agent and RAUC. It provides the same interface as RAUC but with
 * different method names (prefixed with "Update").
 */
class UpdateService {
public:
    UpdateService();
    ~UpdateService();

    /**
     * @brief Initialize the update service and connect to D-Bus
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Start the service main loop
     */
    void run();

    /**
     * @brief Stop the service
     */
    void stop();

private:
    // D-Bus connection for serving our interface
    DBusConnection* service_connection_;

    // D-Bus connection for communicating with RAUC
    DBusConnection* rauc_connection_;

    // Service state
    bool running_;
    bool connected_to_rauc_;

    /**
     * @brief Connect to RAUC D-Bus service
     * @return true if successful, false otherwise
     */
    bool connectToRauc();

    /**
     * @brief Disconnect from RAUC D-Bus service
     */
    void disconnectFromRauc();

    /**
     * @brief Register our D-Bus service
     * @return true if successful, false otherwise
     */
    bool registerService();

    /**
     * @brief Unregister our D-Bus service
     */
    void unregisterService();

    /**
     * @brief D-Bus message handler for incoming method calls
     */
    static DBusHandlerResult messageHandler(DBusConnection* connection,
                                          DBusMessage* message,
                                          void* user_data);

    /**
     * @brief Handle incoming method calls
     */
    DBusHandlerResult handleMethodCall(DBusMessage* message);

    /**
     * @brief Handle property get/set requests
     */
    DBusMessage* handlePropertyCall(DBusMessage* message);

    /**
     * @brief RAUC signal handler for forwarding signals
     */
    static DBusHandlerResult raucSignalHandler(DBusConnection* connection,
                                             DBusMessage* message,
                                             void* user_data);

    /**
     * @brief Forward RAUC signals to our clients
     */
    void forwardRaucSignal(DBusMessage* message);

    // Method implementations that forward to RAUC
    DBusMessage* handleInstall(DBusMessage* message);
    DBusMessage* handleInstallBundle(DBusMessage* message);
    DBusMessage* handleInfo(DBusMessage* message);
    DBusMessage* handleInspectBundle(DBusMessage* message);
    DBusMessage* handleMark(DBusMessage* message);
    DBusMessage* handleGetSlotStatus(DBusMessage* message);
    DBusMessage* handleGetArtifactStatus(DBusMessage* message);
    DBusMessage* handleGetPrimary(DBusMessage* message);

    // Property implementations that forward to RAUC
    DBusMessage* handleGetOperation(DBusMessage* message);
    DBusMessage* handleGetLastError(DBusMessage* message);
    DBusMessage* handleGetProgress(DBusMessage* message);
    DBusMessage* handleGetCompatible(DBusMessage* message);
    DBusMessage* handleGetVariant(DBusMessage* message);
    DBusMessage* handleGetBootSlot(DBusMessage* message);

    /**
     * @brief Forward method call to RAUC
     * @param rauc_method_name The original RAUC method name
     * @param message The incoming D-Bus message
     * @return D-Bus reply message
     */
    DBusMessage* forwardToRauc(const std::string& rauc_method_name, DBusMessage* message);

    /**
     * @brief Get property from RAUC
     * @param property_name The RAUC property name
     * @param original_message The original D-Bus message to create reply for
     * @return D-Bus reply message
     */
    DBusMessage* getRaucProperty(const std::string& property_name, DBusMessage* original_message);

    /**
     * @brief Create error reply
     */
    DBusMessage* createErrorReply(DBusMessage* message, const std::string& error_name, const std::string& error_message);

    /**
     * @brief Log messages with DLT
     */
    void logInfo(const std::string& message);
    void logError(const std::string& message);
    void logDebug(const std::string& message);
};
