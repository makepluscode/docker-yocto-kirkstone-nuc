#include "rauc_client.h"
#include <iostream>
#include <cstring>
#include <dlt/dlt.h>

DLT_DECLARE_CONTEXT(raucClientContext);

RaucClient::RaucClient() : connection_(nullptr), connected_(false) {
    DLT_REGISTER_CONTEXT(raucClientContext, "RAUC", "RAUC client context");
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Initializing RAUC client"));
}

RaucClient::~RaucClient() {
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Destroying RAUC client"));
    disconnect();
    DLT_UNREGISTER_CONTEXT(raucClientContext);
}

bool RaucClient::connect() {
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Connecting to RAUC DBus service"));
    
    DBusError error;
    dbus_error_init(&error);

    connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("DBus connection error: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("DBus connection established"));

    // Add message filter for signals
    dbus_bus_add_match(connection_, 
        "type='signal',interface='de.pengutronix.rauc.Installer'", 
        &error);
    
    if (dbus_error_is_set(&error)) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("DBus match error: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("DBus signal filter added"));

    dbus_connection_add_filter(connection_, messageHandler, this, nullptr);
    connected_ = true;
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Successfully connected to RAUC DBus service"));
    return true;
}

void RaucClient::disconnect() {
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Disconnecting from RAUC DBus service"));
    if (connection_) {
        dbus_connection_remove_filter(connection_, messageHandler, this);
        dbus_connection_close(connection_);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("DBus connection closed"));
    }
    connected_ = false;
}

bool RaucClient::isConnected() const {
    return connected_;
}

bool RaucClient::sendMethodCall(const std::string& method, const std::string& interface) {
    if (!connected_) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Sending method call: "), DLT_STRING(method.c_str()));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to get reply for method: "), DLT_STRING(method.c_str()));
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Method call successful: "), DLT_STRING(method.c_str()));
    dbus_message_unref(reply);
    return true;
}

bool RaucClient::sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface) {
    if (!connected_) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Sending method call: "), DLT_STRING(method.c_str()), 
            DLT_STRING(" with path: "), DLT_STRING(path.c_str()));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    const char* path_str = path.c_str();
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &path_str);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to get reply for method: "), DLT_STRING(method.c_str()));
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Method call successful: "), DLT_STRING(method.c_str()));
    dbus_message_unref(reply);
    return true;
}

bool RaucClient::installBundle(const std::string& bundle_path) {
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Installing bundle: "), DLT_STRING(bundle_path.c_str()));
    return sendMethodCallWithPath("Install", bundle_path);
}

bool RaucClient::getStatus(std::string& status) {
    if (!connected_) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Getting RAUC status"));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        "de.pengutronix.rauc.Installer",
        "GetOperation"
    );

    if (!message) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to get status"));
        return false;
    }

    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter)) {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter)) {
            const char* status_str;
            dbus_message_iter_get_basic(&iter, &status_str);
            status = status_str;
            DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("RAUC status: "), DLT_STRING(status.c_str()));
        }
    }

    dbus_message_unref(reply);
    return true;
}

bool RaucClient::getBootSlot(std::string& boot_slot) {
    if (!connected_) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Getting boot slot"));

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        "de.pengutronix.rauc.Installer",
        "GetBootSlot"
    );

    if (!message) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(raucClientContext, DLT_LOG_ERROR, DLT_STRING("Failed to get boot slot"));
        return false;
    }

    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter)) {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter)) {
            const char* slot_str;
            dbus_message_iter_get_basic(&iter, &slot_str);
            boot_slot = slot_str;
            DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Boot slot: "), DLT_STRING(boot_slot.c_str()));
        }
    }

    dbus_message_unref(reply);
    return true;
}

bool RaucClient::markGood() {
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Marking current slot as good"));
    return sendMethodCall("Mark");
}

bool RaucClient::markBad() {
    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Marking current slot as bad"));
    return sendMethodCall("Mark");
}

bool RaucClient::getBundleInfo(const std::string& bundle_path, std::string& info) {
    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Getting bundle info for: "), DLT_STRING(bundle_path.c_str()));
    return sendMethodCallWithPath("Info", bundle_path);
}

void RaucClient::setProgressCallback(std::function<void(int)> callback) {
    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Setting progress callback"));
    progress_callback_ = callback;
}

void RaucClient::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Setting completed callback"));
    completed_callback_ = callback;
}

DBusHandlerResult RaucClient::messageHandler(DBusConnection* connection, DBusMessage* message, void* user_data) {
    RaucClient* client = static_cast<RaucClient*>(user_data);
    
    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Received DBus signal"));
        client->handleSignal(message);
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void RaucClient::handleSignal(DBusMessage* message) {
    const char* interface = dbus_message_get_interface(message);
    const char* member = dbus_message_get_member(message);
    
    DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Handling signal: "), DLT_STRING(interface), 
            DLT_STRING("."), DLT_STRING(member));
    
    if (strcmp(interface, "de.pengutronix.rauc.Installer") == 0) {
        if (strcmp(member, "Progress") == 0) {
            DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Processing Progress signal"));
            DBusMessageIter iter;
            if (dbus_message_iter_init(message, &iter)) {
                if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&iter)) {
                    int progress;
                    dbus_message_iter_get_basic(&iter, &progress);
                    DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Progress signal received: "), DLT_INT(progress), DLT_STRING("%"));
                    if (progress_callback_) {
                        progress_callback_(progress);
                    }
                }
            }
        } else if (strcmp(member, "Completed") == 0) {
            DLT_LOG(raucClientContext, DLT_LOG_DEBUG, DLT_STRING("Processing Completed signal"));
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
            
            DLT_LOG(raucClientContext, DLT_LOG_INFO, DLT_STRING("Completed signal received. Success: "), DLT_BOOL(success),
                    DLT_STRING(" Message: "), DLT_STRING(message_text.c_str()));
            
            if (completed_callback_) {
                completed_callback_(success, message_text);
            }
        }
    }
} 