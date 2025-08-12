#include "rauc_client.h"
#include <iostream>
#include <cstring>

RaucClient::RaucClient() : connection_(nullptr), connected_(false) {
}

RaucClient::~RaucClient() {
    disconnect();
}

bool RaucClient::connect() {
    DBusError error;
    dbus_error_init(&error);

    connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        std::cerr << "DBus connection error: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }

    // Add message filter for signals
    dbus_bus_add_match(connection_, 
        "type='signal',interface='de.pengutronix.rauc.Installer'", 
        &error);
    
    if (dbus_error_is_set(&error)) {
        std::cerr << "DBus match error: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }

    dbus_connection_add_filter(connection_, messageHandler, this, nullptr);
    connected_ = true;
    std::cout << "Connected to RAUC DBus service" << std::endl;
    return true;
}

void RaucClient::disconnect() {
    if (connection_) {
        dbus_connection_remove_filter(connection_, messageHandler, this);
        dbus_connection_close(connection_);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
    }
    connected_ = false;
}

bool RaucClient::isConnected() const {
    return connected_;
}

bool RaucClient::sendMethodCall(const std::string& method, const std::string& interface) {
    if (!connected_) {
        std::cerr << "Not connected to DBus" << std::endl;
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        std::cerr << "Failed to create DBus message" << std::endl;
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cerr << "Failed to get reply for method: " << method << std::endl;
        return false;
    }

    dbus_message_unref(reply);
    return true;
}

bool RaucClient::sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface) {
    if (!connected_) {
        std::cerr << "Not connected to DBus" << std::endl;
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        std::cerr << "Failed to create DBus message" << std::endl;
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    const char* path_str = path.c_str();
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &path_str);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cerr << "Failed to get reply for method: " << method << std::endl;
        return false;
    }

    dbus_message_unref(reply);
    return true;
}

bool RaucClient::installBundle(const std::string& bundle_path) {
    return sendMethodCallWithPath("Install", bundle_path);
}

bool RaucClient::getStatus(std::string& status) {
    if (!connected_) {
        std::cerr << "Not connected to DBus" << std::endl;
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        "de.pengutronix.rauc.Installer",
        "GetOperation"
    );

    if (!message) {
        std::cerr << "Failed to create DBus message" << std::endl;
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cerr << "Failed to get status" << std::endl;
        return false;
    }

    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter)) {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter)) {
            const char* status_str;
            dbus_message_iter_get_basic(&iter, &status_str);
            status = status_str;
        }
    }

    dbus_message_unref(reply);
    return true;
}

bool RaucClient::getBootSlot(std::string& boot_slot) {
    if (!connected_) {
        std::cerr << "Not connected to DBus" << std::endl;
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/de/pengutronix/rauc",
        "de.pengutronix.rauc.Installer",
        "GetBootSlot"
    );

    if (!message) {
        std::cerr << "Failed to create DBus message" << std::endl;
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cerr << "Failed to get boot slot" << std::endl;
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

bool RaucClient::markGood() {
    return sendMethodCall("Mark");
}

bool RaucClient::markBad() {
    return sendMethodCall("Mark");
}

bool RaucClient::getBundleInfo(const std::string& bundle_path, std::string& info) {
    return sendMethodCallWithPath("Info", bundle_path);
}

void RaucClient::setProgressCallback(std::function<void(int)> callback) {
    progress_callback_ = callback;
}

void RaucClient::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    completed_callback_ = callback;
}

DBusHandlerResult RaucClient::messageHandler(DBusConnection* connection, DBusMessage* message, void* user_data) {
    RaucClient* client = static_cast<RaucClient*>(user_data);
    
    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        client->handleSignal(message);
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void RaucClient::handleSignal(DBusMessage* message) {
    const char* interface = dbus_message_get_interface(message);
    const char* member = dbus_message_get_member(message);
    
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
            
            if (completed_callback_) {
                completed_callback_(success, message_text);
            }
        }
    }
} 