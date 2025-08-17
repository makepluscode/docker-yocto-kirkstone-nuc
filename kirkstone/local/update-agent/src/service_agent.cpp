#include "service_agent.h"
#include <dlt/dlt.h>
#include <cstring>
#include <unistd.h> // For access()

DLT_DECLARE_CONTEXT(dlt_context_updater);

ServiceAgent::ServiceAgent() : connection_(nullptr), connected_(false) {
    DLT_REGISTER_CONTEXT(dlt_context_updater, "SVCA", "Service Agent - Update Service Broker Client");
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Initializing Service Agent (Update Service Broker Client)"));
}

ServiceAgent::~ServiceAgent() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Destroying updater"));
    disconnect();
    DLT_UNREGISTER_CONTEXT(dlt_context_updater);
}

bool ServiceAgent::connect() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Connecting to update service broker (org.freedesktop.UpdateService)"));
    
    DBusError error;
    dbus_error_init(&error);

    connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("DBus connection error: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("DBus connection established"));

    // Test if update service broker is available
    if (!dbus_bus_name_has_owner(connection_, "org.freedesktop.UpdateService", &error)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Update service broker is not available: "), DLT_STRING(error.message));
        dbus_error_free(&error);
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update service broker is available"));

    // Add message filter for update service broker signals
    dbus_bus_add_match(connection_, 
        "type='signal',interface='org.freedesktop.UpdateService'", 
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
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Successfully connected to update service broker DBus"));
    return true;
}

void ServiceAgent::disconnect() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Disconnecting from update service DBus"));
    if (connection_) {
        dbus_connection_remove_filter(connection_, messageHandler, this);
        // dbus_connection_close(connection_); // 공유 연결이므로 닫으면 안 됨
        dbus_connection_unref(connection_);
        connection_ = nullptr;
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("DBus connection closed"));
    }
    connected_ = false;
}

bool ServiceAgent::isConnected() const {
    return connected_;
}

bool ServiceAgent::sendMethodCall(const std::string& method, const std::string& interface) {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Sending method call: "), DLT_STRING(method.c_str()), DLT_STRING(" to interface: "), DLT_STRING(interface.c_str()));

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
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

bool ServiceAgent::sendMethodCallWithPath(const std::string& method, const std::string& path, const std::string& interface) {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Sending method call: "), DLT_STRING(method.c_str()), DLT_STRING(" with path: "), DLT_STRING(path.c_str()), DLT_STRING(" to interface: "), DLT_STRING(interface.c_str()));

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
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

bool ServiceAgent::checkService() {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Checking update service status..."));

    // Try to get update service status to verify service is responding
    std::string status;
    if (getStatus(status)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update service is responding, current status: "), DLT_STRING(status.c_str()));
        return true;
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("First status check failed, attempting to reconnect..."));
        
        // Try to reconnect and retry once
        disconnect();
        if (connect()) {
            DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Reconnected successfully, retrying status check..."));
            if (getStatus(status)) {
                DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update service is responding after reconnect, status: "), DLT_STRING(status.c_str()));
                return true;
            }
        }
        
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Update service is not responding even after reconnect"));
        return false;
    }
}

bool ServiceAgent::installBundle(const std::string& bundle_path) {
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
    
    // Check update service status before attempting installation
    if (!checkService()) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Update service is not available, cannot install bundle"));
        return false;
    }
    
    bool result = sendMethodCallWithPath("Install", bundle_path, "org.freedesktop.UpdateService");
    
    if (result) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Bundle installation started successfully"));
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Bundle installation failed to start"));
    }
    
    return result;
}

bool ServiceAgent::getStatus(std::string& status) {
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting update service status"));

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
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
    const char* interface_name = "org.freedesktop.UpdateService";
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

    DBusError error;
    dbus_error_init(&error);
    
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("=== Sending Properties.Get call to update-service ==="));
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Service: org.freedesktop.UpdateService"));
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Interface: org.freedesktop.DBus.Properties"));
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Property: Operation"));
    
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 5000, &error); // Reduced to 5 seconds
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("=== Properties.Get call FAILED ==="));
        if (dbus_error_is_set(&error)) {
            DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("D-Bus error name: "), DLT_STRING(error.name));
            DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("D-Bus error message: "), DLT_STRING(error.message));
            dbus_error_free(&error);
        } else {
            DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("No D-Bus error info available (likely timeout)"));
        }
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get status after 30 second timeout"));
        return false;
    }
    
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("=== Properties.Get call succeeded ==="));

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
                DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Update service status: "), DLT_STRING(status.c_str()));
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

bool ServiceAgent::getBootSlot(std::string& boot_slot) {
    if (!connected_) {
        return false;
    }
    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
        "org.freedesktop.UpdateService",
        "GetPrimary"
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

bool ServiceAgent::markGood() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Marking current slot as good"));
    
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
        "org.freedesktop.UpdateService",
        "Mark"
    );

    if (!message) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    
    // Add state parameter
    const char* state = "good";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &state)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append state parameter"));
        dbus_message_unref(message);
        return false;
    }
    
    // Add slot_identifier parameter
    const char* slot_identifier = "booted";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &slot_identifier)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append slot_identifier parameter"));
        dbus_message_unref(message);
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 30000, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get reply for Mark good"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Mark good successful"));
    dbus_message_unref(reply);
    return true;
}

bool ServiceAgent::markBad() {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Marking current slot as bad"));
    
    if (!connected_) {
        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("Not connected to DBus"));
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
        "org.freedesktop.UpdateService",
        "Mark"
    );

    if (!message) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to create DBus message"));
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);
    
    // Add state parameter
    const char* state = "bad";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &state)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append state parameter"));
        dbus_message_unref(message);
        return false;
    }
    
    // Add slot_identifier parameter
    const char* slot_identifier = "booted";
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &slot_identifier)) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to append slot_identifier parameter"));
        dbus_message_unref(message);
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection_, message, 30000, nullptr);
    dbus_message_unref(message);

    if (!reply) {
        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get reply for Mark bad"));
        return false;
    }

    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Mark bad successful"));
    dbus_message_unref(reply);
    return true;
}

bool ServiceAgent::getBundleInfo(const std::string& bundle_path, std::string& info) {
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Getting bundle info for: "), DLT_STRING(bundle_path.c_str()));
    return sendMethodCallWithPath("Info", bundle_path, "org.freedesktop.UpdateService");
}

void ServiceAgent::setProgressCallback(std::function<void(int)> callback) {
    progress_callback_ = callback;
}

void ServiceAgent::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    completed_callback_ = callback;
}

DBusHandlerResult ServiceAgent::messageHandler(DBusConnection* connection, DBusMessage* message, void* user_data) {
    ServiceAgent* client = static_cast<ServiceAgent*>(user_data);
    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        client->handleSignal(message);
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void ServiceAgent::handleSignal(DBusMessage* message) {
    const char* interface = dbus_message_get_interface(message);
    const char* member = dbus_message_get_member(message);
    const char* sender = dbus_message_get_sender(message);
    
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("=== SIGNAL RECEIVED ==="));
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Interface: "), DLT_STRING(interface ? interface : "null"));
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Member: "), DLT_STRING(member ? member : "null"));
    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Sender: "), DLT_STRING(sender ? sender : "null"));
    
    if (strcmp(interface, "org.freedesktop.UpdateService") == 0) {
        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Processing UpdateService signal: "), DLT_STRING(member));
        
        if (strcmp(member, "Progress") == 0) {
            DBusMessageIter iter;
            if (dbus_message_iter_init(message, &iter)) {
                if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&iter)) {
                    int progress;
                    dbus_message_iter_get_basic(&iter, &progress);
                    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Progress signal received: "), DLT_INT(progress), DLT_STRING("%"));
                    
                    if (progress_callback_) {
                        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Calling progress callback"));
                        progress_callback_(progress);
                    } else {
                        DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("No progress callback registered"));
                    }
                } else {
                    DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Progress signal has wrong argument type"));
                }
            } else {
                DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get Progress signal arguments"));
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
                    DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Completed signal - success: "), DLT_BOOL(success));
                } else {
                    DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Completed signal has wrong first argument type"));
                }
                
                if (dbus_message_iter_next(&iter)) {
                    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter)) {
                        const char* msg;
                        dbus_message_iter_get_basic(&iter, &msg);
                        message_text = msg;
                        DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Completed signal - message: "), DLT_STRING(message_text.c_str()));
                    } else {
                        DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Completed signal has wrong second argument type"));
                    }
                } else {
                    DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Completed signal missing second argument"));
                }
            } else {
                DLT_LOG(dlt_context_updater, DLT_LOG_ERROR, DLT_STRING("Failed to get Completed signal arguments"));
            }
            
            DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Completed signal: Success="), DLT_BOOL(success), DLT_STRING(", Message: "), DLT_STRING(message_text.c_str()));
            
            if (completed_callback_) {
                DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Calling completed callback"));
                completed_callback_(success, message_text);
            } else {
                DLT_LOG(dlt_context_updater, DLT_LOG_WARN, DLT_STRING("No completed callback registered"));
            }
        } else {
            DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Unknown UpdateService signal: "), DLT_STRING(member));
        }
    } else {
        DLT_LOG(dlt_context_updater, DLT_LOG_DEBUG, DLT_STRING("Ignoring signal from interface: "), DLT_STRING(interface ? interface : "null"));
    }
}

void ServiceAgent::processMessages() {
    if (!connected_ || !connection_) {
        return;
    }
    
    // Process pending D-Bus messages (non-blocking)
    int dispatch_count = 0;
    while (dbus_connection_get_dispatch_status(connection_) == DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(connection_);
        dispatch_count++;
    }
    
    // Read new messages from the socket (non-blocking)
    dbus_connection_read_write(connection_, 0);
    
    // Log message processing periodically
    if (dispatch_count > 0) {
        DLT_LOG(dlt_context_updater, DLT_LOG_DEBUG, DLT_STRING("Processed "), DLT_INT(dispatch_count), DLT_STRING(" D-Bus messages"));
    }
} 
