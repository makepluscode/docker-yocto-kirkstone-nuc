#include "updater.h"
#include <dlt/dlt.h>
#include <cstring>
#include <unistd.h> // For access()

DLT_DECLARE_CONTEXT(dlt_context_updater);

Updater::Updater() : connection_(nullptr), connected_(false) {
    DLT_REGISTER_CONTEXT(dlt_context_updater, "UPDATER", "Updater Logic");
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Initializing updater"));
}

Updater::~Updater() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Destroying updater"));
    disconnect();
    DLT_UNREGISTER_CONTEXT(dlt_context_updater);
}

bool Updater::connect() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Connecting to update service"));
    
    DBusError error;
    dbus_error_init(&error);

    connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("DBus connection error: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("DBus connection established"));

    // Test if RAUC service is available
    if (!dbus_bus_name_has_owner(connection_, "de.pengutronix.rauc", &error)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("RAUC service is not available: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("RAUC service is available"));

    // Add message filter for signals
    dbus_bus_add_match(connection_, 
        "type='signal',interface='de.pengutronix.rauc.Installer'", 
        &error);
    
    if (dbus_error_is_set(&error)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("DBus match error: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("DBus signal filter added"));

    dbus_connection_add_filter(connection_, messageHandler, this, nullptr);
    connected_ = true;
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Successfully connected to RAUC DBus service"));
    return true;
}

void Updater::disconnect() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Disconnecting from RAUC DBus service"));
    if (connection_) {
        dbus_connection_remove_filter(connection_, messageHandler, this);
        // dbus_connection_close(connection_); // 공유 연결이므로 닫으면 안 됨
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("DBus connection closed"));
    }
    connected_ = false;
}

bool Updater::isConnected() const {
    return connected_;
}

bool Updater::sendMethodCall(const std::string& method, const std::string& interface) {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Sending method call: "), DLT_STRING(method.c_str()), DLT_STRING(" to interface: "), DLT_STRING(interface.c_str()));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_DEBUG, DLT_STRING("DBus message created, sending..."));

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get reply for method: "), DLT_STRING(method.c_str()));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Method call successful: "), DLT_STRING(method.c_str()));
    dbus_message_unref(reply);
    return true;
}

bool Updater::sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface) {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Sending method call: "), DLT_STRING(method.c_str()), DLT_STRING(" with path: "), DLT_STRING(path.c_str()), DLT_STRING(" to interface: "), DLT_STRING(interface.c_str()));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    const char* path_str = path.c_str();
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &path_str)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append path to DBus message"));
        dbus_message_unref(message);
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_DEBUG, DLT_STRING("DBus message created with path, sending..."));

    // Send with timeout (30 seconds)
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 30000, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get reply for method: "), DLT_STRING(method.c_str()), DLT_STRING(" (timeout or no response)"));
        return false;
    }

    // Check if reply is an error
    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
        DBusError error;
        dbus_error_init(&error);
        if (dbus_set_error_from_message(&error, reply)) {
            DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("DBus error reply: "), DLT_STRING(error.name), DLT_STRING(" - "), DLT_STRING(error.message));
            dbus_error_free(&error);
        }
        dbus_message_unref(reply);
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Method call successful: "), DLT_STRING(method.c_str()));
    dbus_message_unref(reply);
    return true;
}

bool Updater::checkService() {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Checking RAUC service status..."));

    // Try to get RAUC status to verify service is responding
    std::string status;
    if (getStatus(status)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("RAUC service is responding, current status: "), DLT_STRING(status.c_str()));
        return true;
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("RAUC service is not responding"));
        return false;
    }
}

bool Updater::installBundle(const std::string& bundle_path) {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Installing bundle: "), DLT_STRING(bundle_path.c_str()));
    
    // Check if file exists and is readable
    if (access(bundle_path.c_str(), F_OK) != 0) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Bundle file does not exist: "), DLT_STRING(bundle_path.c_str()));
        return false;
    }
    
    if (access(bundle_path.c_str(), R_OK) != 0) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Bundle file is not readable: "), DLT_STRING(bundle_path.c_str()));
        return false;
    }
    
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Bundle file exists and is readable"));
    
    // Check RAUC service status before attempting installation
    if (!checkService()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("RAUC service is not available, cannot install bundle"));
        return false;
    }
    
    bool result = sendMethodCallWithPath("Install", bundle_path, "de.pengutronix.rauc.Installer");
    
    if (result) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Bundle installation started successfully"));
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Bundle installation failed to start"));
    }
    
    return result;
}

bool Updater::getStatus(std::string& status) {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting RAUC status"));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/",
        "org.freedesktop.DBus.Properties",
        "Get"
    );

    if (!message) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    
    // Add interface name
    const char* interface_name = "de.pengutronix.rauc.Installer";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface_name)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append interface name"));
        dbus_message_unref(message);
        return false;
    }
    
    // Add property name
    const char* property_name = "Operation";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &property_name)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append property name"));
        dbus_message_unref(message);
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_DEBUG, DLT_STRING("DBus message created, sending..."));

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 30000, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get status"));
        return false;
    }

    // Parse the variant response
    DBusMessageIter reply_iter;
    if (dbus_message_iter_init(reply, &reply_iter)) {
        if (DBUS_TYPE_VARIANT == dbus_message_iter_get_arg_type(&reply_iter)) {
            DBusMessageIter variant_iter;
            dbus_message_iter_recurse(&reply_iter, &variant_iter);
            
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variant_iter)) {
                const char* status_str;
                dbus_message_iter_get_basic(&variant_iter, &status_str);
                status = status_str;
                DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("RAUC status: "), DLT_STRING(status.c_str()));
            } else {
                DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Unexpected variant type for Operation property"));
            }
        } else {
            DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Unexpected reply type"));
        }
    }

    dbus_message_unref(reply);
    return true;
}

bool Updater::getBootSlot(std::string& boot_slot) {
    if (!connected_) {
        return false;
    }
    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        "de.pengutronix.rauc.Installer",
        "GetBootSlot"
    );
    if (!message) {
        return false;
    }
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);
    if (!reply) {
        return false;
    }
    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter)) {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter)) {
            const char* slot_str;
            dbus_message_iter_get_basic(&iter, &slot_str);
            boot_slot = slot_str;
        }
    }
    dbus_message_unref(reply);
    return true;
}

bool Updater::markGood() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Marking current slot as good"));
    return sendMethodCall("Mark", "de.pengutronix.rauc.Installer");
}

bool Updater::markBad() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Marking current slot as bad"));
    return sendMethodCall("Mark", "de.pengutronix.rauc.Installer");
}

bool Updater::getBundleInfo(const std::string& bundle_path, std::string& info) {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting bundle info for: "), DLT_STRING(bundle_path.c_str()));
    return sendMethodCallWithPath("Info", bundle_path, "de.pengutronix.rauc.Installer");
}

void Updater::setProgressCallback(std::function<void(int)> callback) {
    progress_callback_ = callback;
}

void Updater::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    completed_callback_ = callback;
}

DBusHandlerResult Updater::messageHandler(DBusConnection* connection, DBusMessage* message, void* user_data) {
    Updater* client = static_cast<Updater*>(user_data);
    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        client->handleSignal(message);
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void Updater::handleSignal(DBusMessage* message) {
    const char* interface = dbus_message_get_interface(message);
    const char* member = dbus_message_get_member(message);
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Received signal: "), DLT_STRING(interface), DLT_STRING("."), DLT_STRING(member));
    if (strcmp(interface, "de.pengutronix.rauc.Installer") == 0) {
        if (strcmp(member, "Progress") == 0) {
            DBusMessageIter iter;
            if (dbus_message_iter_init(message, &iter)) {
                if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&iter)) {
                    int progress;
                    dbus_message_iter_get_basic(&iter, &progress);
                    if (progress_callback_) {
                        progress_callback_(progress);
                    }
                }
            }
        } else if (strcmp(member, "Completed") == 0) {
            DBusMessageIter iter;
            bool success = false;
            std::string message_text;
            if (dbus_message_iter_init(message, &iter)) {
                if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter)) {
                    dbus_bool_t result;
                    dbus_message_iter_get_basic(&iter, &result);
                    success = result;
                }
                if (dbus_message_iter_next(&iter)) {
                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter)) {
                        const char* msg;
                        dbus_message_iter_get_basic(&iter, &msg);
                        message_text = msg;
                    }
                }
            }
            DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Completed signal: Success="), DLT_BOOL(success), DLT_STRING(", Message: "), DLT_STRING(message_text.c_str()));
            if (completed_callback_) {
                completed_callback_(success, message_text);
            }
        }
    }
} 