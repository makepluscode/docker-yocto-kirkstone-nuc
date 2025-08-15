#include "rauc_client.h"
#include <iostream>
#include <cstring>
#include <unistd.h> // For access()

RaucClient::RaucClient() : connection_(nullptr), connected_(false) {
    std::cout << "Initializing RAUC client" << std::endl;
}

RaucClient::~RaucClient() {
    std::cout << "Destroying RAUC client" << std::endl;
    disconnect();
}

bool RaucClient::connect() {
    std::cout << "Connecting to RAUC DBus service" << std::endl;
    
    DBusError error;
    dbus_error_init(&error);

    connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        std::cout << "DBus connection error: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }

    std::cout << "DBus connection established" << std::endl;

    // Test if RAUC service is available
    if (!dbus_bus_name_has_owner(connection_, "de.pengutronix.rauc", &error)) {
        std::cout << "RAUC service is not available: " << error.message << std::endl;
        dbus_error_free(&error);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        return false;
    }

    std::cout << "RAUC service is available" << std::endl;

    // Add message filter for signals
    dbus_bus_add_match(connection_, 
        "type='signal',interface='de.pengutronix.rauc.Installer'", 
        &error);
    
    if (dbus_error_is_set(&error)) {
        std::cout << "DBus match error: " << error.message << std::endl;
        dbus_error_free(&error);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        return false;
    }

    std::cout << "DBus signal filter added" << std::endl;

    dbus_connection_add_filter(connection_, messageHandler, this, nullptr);
    connected_ = true;
    std::cout << "Successfully connected to RAUC DBus service" << std::endl;
    return true;
}

void RaucClient::disconnect() {
    std::cout << "Disconnecting from RAUC DBus service" << std::endl;
    if (connection_) {
        dbus_connection_remove_filter(connection_, messageHandler, this);
        // dbus_connection_close(connection_); // 공유 연결이므로 닫으면 안 됨
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        std::cout << "DBus connection closed" << std::endl;
    }
    connected_ = false;
}

bool RaucClient::isConnected() const {
    return connected_;
}

bool RaucClient::sendMethodCall(const std::string& method, const std::string& interface) {
    if (!connected_) {
        std::cout << "Not connected to DBus" << std::endl;
        return false;
    }

    std::cout << "Sending method call: " << method << " to interface: " << interface << std::endl;

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        std::cout << "Failed to create DBus message" << std::endl;
        return false;
    }

    std::cout << "DBus message created, sending..." << std::endl;

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, -1, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cout << "Failed to get reply for method: " << method << std::endl;
        return false;
    }

    std::cout << "Method call successful: " << method << std::endl;
    dbus_message_unref(reply);
    return true;
}

bool RaucClient::sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface) {
    if (!connected_) {
        std::cout << "Not connected to DBus" << std::endl;
        return false;
    }

    std::cout << "Sending method call: " << method << " with path: " << path << " to interface: " << interface << std::endl;

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/",
        interface.c_str(),
        method.c_str()
    );

    if (!message) {
        std::cout << "Failed to create DBus message" << std::endl;
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    const char* path_str = path.c_str();
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &path_str)) {
        std::cout << "Failed to append path to DBus message" << std::endl;
        dbus_message_unref(message);
        return false;
    }

    std::cout << "DBus message created with path, sending..." << std::endl;

    // Send with timeout (30 seconds)
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 30000, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cout << "Failed to get reply for method: " << method << " (timeout or no response)" << std::endl;
        return false;
    }

    // Check if reply is an error
    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
        DBusError error;
        dbus_error_init(&error);
        if (dbus_set_error_from_message(&error, reply)) {
            std::cout << "DBus error reply: " << error.name << " - " << error.message << std::endl;
            dbus_error_free(&error);
        }
        dbus_message_unref(reply);
        return false;
    }

    std::cout << "Method call successful: " << method << std::endl;
    dbus_message_unref(reply);
    return true;
}

bool RaucClient::checkRaucService() {
    if (!connected_) {
        std::cout << "Not connected to DBus" << std::endl;
        return false;
    }

    std::cout << "Checking RAUC service status..." << std::endl;

    // Try to get RAUC status to verify service is responding
    std::string status;
    if (getStatus(status)) {
        std::cout << "RAUC service is responding, current status: " << status << std::endl;
        return true;
    } else {
        std::cout << "RAUC service is not responding" << std::endl;
        return false;
    }
}

bool RaucClient::installBundle(const std::string& bundle_path) {
    std::cout << "Installing bundle: " << bundle_path << std::endl;
    
    // Check if file exists and is readable
    if (access(bundle_path.c_str(), F_OK) != 0) {
        std::cout << "Bundle file does not exist: " << bundle_path << std::endl;
        return false;
    }
    
    if (access(bundle_path.c_str(), R_OK) != 0) {
        std::cout << "Bundle file is not readable: " << bundle_path << std::endl;
        return false;
    }
    
    std::cout << "Bundle file exists and is readable" << std::endl;
    
    // Check RAUC service status before attempting installation
    if (!checkRaucService()) {
        std::cout << "RAUC service is not available, cannot install bundle" << std::endl;
        return false;
    }
    
    bool result = sendMethodCallWithPath("Install", bundle_path, "de.pengutronix.rauc.Installer");
    
    if (result) {
        std::cout << "Bundle installation started successfully" << std::endl;
    } else {
        std::cout << "Bundle installation failed to start" << std::endl;
    }
    
    return result;
}

bool RaucClient::getStatus(std::string& status) {
    if (!connected_) {
        std::cout << "Not connected to DBus" << std::endl;
        return false;
    }

    std::cout << "Getting RAUC status" << std::endl;

    DBusMessage* message = dbus_message_new_method_call(
        "de.pengutronix.rauc",
        "/",
        "org.freedesktop.DBus.Properties",
        "Get"
    );

    if (!message) {
        std::cout << "Failed to create DBus message" << std::endl;
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    
    // Add interface name
    const char* interface_name = "de.pengutronix.rauc.Installer";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface_name)) {
        std::cout << "Failed to append interface name" << std::endl;
        dbus_message_unref(message);
        return false;
    }
    
    // Add property name
    const char* property_name = "Operation";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &property_name)) {
        std::cout << "Failed to append property name" << std::endl;
        dbus_message_unref(message);
        return false;
    }

    std::cout << "DBus message created, sending..." << std::endl;

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 30000, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        std::cout << "Failed to get status" << std::endl;
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
                std::cout << "RAUC status: " << status << std::endl;
            } else {
                std::cout << "Unexpected variant type for Operation property" << std::endl;
            }
        } else {
            std::cout << "Unexpected reply type" << std::endl;
        }
    }

    dbus_message_unref(reply);
    return true;
}

bool RaucClient::getBootSlot(std::string& boot_slot) {
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

bool RaucClient::markGood() {
    std::cout << "Marking current slot as good" << std::endl;
    return sendMethodCall("Mark", "de.pengutronix.rauc.Installer");
}

bool RaucClient::markBad() {
    std::cout << "Marking current slot as bad" << std::endl;
    return sendMethodCall("Mark", "de.pengutronix.rauc.Installer");
}

bool RaucClient::getBundleInfo(const std::string& bundle_path, std::string& info) {
    std::cout << "Getting bundle info for: " << bundle_path << std::endl;
    return sendMethodCallWithPath("Info", bundle_path, "de.pengutronix.rauc.Installer");
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
    std::cout << "Received signal: " << interface << "." << member << std::endl;
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
            std::cout << "Completed signal: Success=" << success << ", Message: " << message_text << std::endl;
            if (completed_callback_) {
                completed_callback_(success, message_text);
            }
        }
    }
} 