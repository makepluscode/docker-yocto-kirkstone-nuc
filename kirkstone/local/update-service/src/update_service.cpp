#include "update_service.h"
#include <dlt/dlt.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

DLT_DECLARE_CONTEXT(dlt_context_service);

// D-Bus service constants following freedesktop.org conventions
static const char* SERVICE_NAME = "org.freedesktop.UpdateService";
static const char* OBJECT_PATH = "/org/freedesktop/UpdateService";
static const char* INTERFACE_NAME = "org.freedesktop.UpdateService";

// RAUC D-Bus constants
static const char* RAUC_SERVICE_NAME = "de.pengutronix.rauc";
static const char* RAUC_OBJECT_PATH = "/";
static const char* RAUC_INTERFACE_NAME = "de.pengutronix.rauc.Installer";
static const char* RAUC_PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

UpdateService::UpdateService()
    : service_connection_(nullptr)
    , rauc_connection_(nullptr)
    , running_(false)
    , connected_to_rauc_(false) {

    DLT_REGISTER_CONTEXT(dlt_context_service, "USVC", "Update Service");
    logInfo("Update Service initializing");
}

UpdateService::~UpdateService() {
    stop();
    DLT_UNREGISTER_CONTEXT(dlt_context_service);
}

bool UpdateService::initialize() {
    logInfo("Initializing Update Service");

    // Initialize D-Bus error
    DBusError error;
    dbus_error_init(&error);

    // Connect to system bus for our service
    service_connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        logError("Failed to connect to D-Bus system bus: " + std::string(error.message));
        dbus_error_free(&error);
        return false;
    }

    // Connect to RAUC (non-blocking - will retry in main loop if needed)
    if (!connectToRauc()) {
        logInfo("RAUC service not immediately available - will retry during operation");
        connected_to_rauc_ = false;
    }

    // Register our service
    if (!registerService()) {
        logError("Failed to register Update Service");
        return false;
    }

    logInfo("Update Service initialized successfully");
    return true;
}

bool UpdateService::connectToRauc() {
    logInfo("Connecting to RAUC service");

    DBusError error;
    dbus_error_init(&error);

    // Get separate connection for RAUC communication
    rauc_connection_ = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        logError("Failed to connect to D-Bus for RAUC: " + std::string(error.message));
        dbus_error_free(&error);
        return false;
    }

    // Check if RAUC service is available
    if (!dbus_bus_name_has_owner(rauc_connection_, RAUC_SERVICE_NAME, &error)) {
        logError("RAUC service is not available: " + std::string(error.message));
        dbus_error_free(&error);
        return false;
    }

    // Add signal filter for RAUC signals - EXACTLY like the working version
    dbus_bus_add_match(rauc_connection_,
        "type='signal',interface='de.pengutronix.rauc.Installer'",
        &error);

    if (dbus_error_is_set(&error)) {
        logError("Failed to add RAUC signal filter: " + std::string(error.message));
        dbus_error_free(&error);
        return false;
    }

    logInfo("Added RAUC signal filter: type='signal',interface='de.pengutronix.rauc.Installer'");

    // Add signal handler
    dbus_connection_add_filter(rauc_connection_, raucSignalHandler, this, nullptr);
    logInfo("RAUC signal handler registered");

    connected_to_rauc_ = true;
    logInfo("Successfully connected to RAUC service");
    return true;
}

void UpdateService::disconnectFromRauc() {
    if (rauc_connection_) {
        dbus_connection_remove_filter(rauc_connection_, raucSignalHandler, this);
        dbus_connection_unref(rauc_connection_);
        rauc_connection_ = nullptr;
        connected_to_rauc_ = false;
        logInfo("Disconnected from RAUC service");
    }
}

bool UpdateService::registerService() {
    logInfo("Registering Update Service D-Bus interface");

    DBusError error;
    dbus_error_init(&error);

    // Request service name
    int result = dbus_bus_request_name(service_connection_, SERVICE_NAME,
                                      DBUS_NAME_FLAG_REPLACE_EXISTING,
                                      &error);

    if (dbus_error_is_set(&error)) {
        logError("Failed to request service name: " + std::string(error.message));
        dbus_error_free(&error);
        return false;
    }

    if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        logError("Failed to become primary owner of service name");
        return false;
    }

    // Add object path handler
    static const DBusObjectPathVTable vtable = {
        nullptr,        // unregister function
        messageHandler, // message function
        nullptr,        // dbus_internal_pad1
        nullptr,        // dbus_internal_pad2
        nullptr,        // dbus_internal_pad3
        nullptr         // dbus_internal_pad4
    };

    if (!dbus_connection_register_object_path(service_connection_, OBJECT_PATH,
                                            &vtable, this)) {
        logError("Failed to register object path");
        return false;
    }

    logInfo("Update Service registered successfully");
    return true;
}

void UpdateService::unregisterService() {
    if (service_connection_) {
        dbus_connection_unregister_object_path(service_connection_, OBJECT_PATH);
        dbus_bus_release_name(service_connection_, SERVICE_NAME, nullptr);
        logInfo("Update Service unregistered");
    }
}

void UpdateService::run() {
    logInfo("Starting Update Service main loop");
    logInfo("Service connection active: " + std::string(service_connection_ ? "YES" : "NO"));
    logInfo("RAUC connection active: " + std::string(connected_to_rauc_ ? "YES" : "NO"));
    running_ = true;

    int loop_count = 0;
    while (running_) {
        // Handle D-Bus messages for our service
        dbus_connection_read_write_dispatch(service_connection_, 100);

        // Handle RAUC signals with more detailed logging
        if (connected_to_rauc_) {
            // Process RAUC messages more frequently
            int rauc_dispatch_count = 0;
            while (dbus_connection_get_dispatch_status(rauc_connection_) == DBUS_DISPATCH_DATA_REMAINS) {
                dbus_connection_dispatch(rauc_connection_);
                rauc_dispatch_count++;
            }

            // Read new RAUC messages
            dbus_connection_read_write(rauc_connection_, 10);

            // Log RAUC message processing periodically
            if (rauc_dispatch_count > 0 && loop_count % 100 == 0) {
                logInfo("RAUC messages processed: " + std::to_string(rauc_dispatch_count));
            }
        } else {
            // Log connection loss periodically
            if (loop_count % 500 == 0) { // Every ~5 seconds
                logError("RAUC connection lost - attempting to reconnect...");
                if (connectToRauc()) {
                    logInfo("RAUC connection restored successfully");
                } else {
                    logError("Failed to restore RAUC connection");
                }
            }
        }

        // Periodic status logging (every ~10 seconds for debugging)
        if (loop_count % 1000 == 0) {
            logInfo("=== Update Service Status ===");
            logInfo("Service running: " + std::string(running_ ? "YES" : "NO"));
            logInfo("Service connection: " + std::string(service_connection_ ? "ACTIVE" : "INACTIVE"));
            logInfo("RAUC connection: " + std::string(connected_to_rauc_ ? "ACTIVE" : "INACTIVE"));
            logInfo("Loop count: " + std::to_string(loop_count));
            logInfo("=== WAITING FOR D-BUS MESSAGES ===");
        }

        // Small sleep to prevent busy waiting
        usleep(10000); // 10ms
        loop_count++;
    }

    logInfo("Update Service main loop stopped");
}

void UpdateService::stop() {
    logInfo("Stopping Update Service");
    running_ = false;

    disconnectFromRauc();
    unregisterService();

    if (service_connection_) {
        dbus_connection_unref(service_connection_);
        service_connection_ = nullptr;
    }
}

DBusHandlerResult UpdateService::messageHandler(DBusConnection* connection,
                                               DBusMessage* message,
                                               void* user_data) {
    UpdateService* service = static_cast<UpdateService*>(user_data);
    service->logInfo("=== MESSAGE HANDLER CALLED ===");
    return service->handleMethodCall(message);
}

DBusHandlerResult UpdateService::handleMethodCall(DBusMessage* message) {
    const char* interface = dbus_message_get_interface(message);
    const char* member = dbus_message_get_member(message);
    const char* path = dbus_message_get_path(message);
    const char* sender = dbus_message_get_sender(message);

    logInfo("=== HANDLE METHOD CALL ENTERED ===");
    logInfo("Interface: " + std::string(interface ? interface : "null"));
    logInfo("Member: " + std::string(member ? member : "null"));
    logInfo("Path: " + std::string(path ? path : "null"));
    logInfo("Sender: " + std::string(sender ? sender : "null"));

    // Handle our interface methods
    if (interface && strcmp(interface, INTERFACE_NAME) == 0) {
        DBusMessage* reply = nullptr;

        if (strcmp(member, "Install") == 0) {
            reply = handleInstall(message);
        } else if (strcmp(member, "InstallBundle") == 0) {
            reply = handleInstallBundle(message);
        } else if (strcmp(member, "Info") == 0) {
            reply = handleInfo(message);
        } else if (strcmp(member, "InspectBundle") == 0) {
            reply = handleInspectBundle(message);
        } else if (strcmp(member, "Mark") == 0) {
            reply = handleMark(message);
        } else if (strcmp(member, "GetSlotStatus") == 0) {
            reply = handleGetSlotStatus(message);
        } else if (strcmp(member, "GetArtifactStatus") == 0) {
            reply = handleGetArtifactStatus(message);
        } else if (strcmp(member, "GetPrimary") == 0) {
            reply = handleGetPrimary(message);
        }

        if (reply) {
            dbus_connection_send(service_connection_, reply, nullptr);
            dbus_message_unref(reply);
            logDebug("Reply sent for method: " + std::string(member ? member : "unknown"));
            return DBUS_HANDLER_RESULT_HANDLED;
        } else {
            logError("Failed to create reply for method: " + std::string(member ? member : "unknown"));
        }
    }

    // Handle properties interface
    if (interface && strcmp(interface, "org.freedesktop.DBus.Properties") == 0) {
        logInfo("=== PROCESSING PROPERTIES INTERFACE ===");
        DBusMessage* reply = handlePropertyCall(message);
        if (reply) {
            dbus_connection_send(service_connection_, reply, nullptr);
            dbus_message_unref(reply);
            logInfo("=== PROPERTIES REPLY SENT ===");
            return DBUS_HANDLER_RESULT_HANDLED;
        } else {
            logError("=== PROPERTIES CALL RETURNED NULL ===");
        }
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusMessage* UpdateService::handlePropertyCall(DBusMessage* message) {
    const char* member = dbus_message_get_member(message);

    if (strcmp(member, "Get") == 0) {
        DBusMessageIter iter;
        const char* interface_name;
        const char* property_name;

        if (!dbus_message_iter_init(message, &iter) ||
            dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
            return createErrorReply(message, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid arguments");
        }

        dbus_message_iter_get_basic(&iter, &interface_name);

        if (!dbus_message_iter_next(&iter) ||
            dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
            return createErrorReply(message, "org.freedesktop.DBus.Error.InvalidArgs", "Invalid arguments");
        }

        dbus_message_iter_get_basic(&iter, &property_name);

        // Handle our properties
        if (strcmp(interface_name, INTERFACE_NAME) == 0) {
            if (strcmp(property_name, "Operation") == 0) {
                return handleGetOperation(message);
            } else if (strcmp(property_name, "LastError") == 0) {
                return handleGetLastError(message);
            } else if (strcmp(property_name, "Progress") == 0) {
                return handleGetProgress(message);
            } else if (strcmp(property_name, "Compatible") == 0) {
                return handleGetCompatible(message);
            } else if (strcmp(property_name, "Variant") == 0) {
                return handleGetVariant(message);
            } else if (strcmp(property_name, "BootSlot") == 0) {
                return handleGetBootSlot(message);
            }
        }
    }

    return createErrorReply(message, "org.freedesktop.DBus.Error.UnknownProperty", "Unknown property");
}

DBusHandlerResult UpdateService::raucSignalHandler(DBusConnection* connection,
                                                  DBusMessage* message,
                                                  void* user_data) {
    UpdateService* service = static_cast<UpdateService*>(user_data);

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        const char* interface = dbus_message_get_interface(message);
        const char* member = dbus_message_get_member(message);
        const char* sender = dbus_message_get_sender(message);

        service->logInfo("=== RAUC SIGNAL RECEIVED ===");
        service->logInfo("Interface: " + std::string(interface ? interface : "null"));
        service->logInfo("Member: " + std::string(member ? member : "null"));
        service->logInfo("Sender: " + std::string(sender ? sender : "null"));

        service->forwardRaucSignal(message);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void UpdateService::forwardRaucSignal(DBusMessage* message) {
    const char* interface = dbus_message_get_interface(message);
    const char* member = dbus_message_get_member(message);

    logInfo("=== FORWARDING RAUC SIGNAL ===");
    logInfo("Original interface: " + std::string(interface ? interface : "null"));
    logInfo("Original member: " + std::string(member ? member : "null"));

    // Check if this is a RAUC signal - EXACTLY like the working version
    if (!interface || strcmp(interface, "de.pengutronix.rauc.Installer") != 0) {
        logDebug("Not a RAUC Installer signal, ignoring: " + std::string(interface ? interface : "unknown"));
        return;
    }

    logInfo("Processing RAUC signal: " + std::string(member));

    // Create new signal with our interface name and updated member name
    DBusMessage* signal = nullptr;

    if (strcmp(member, "Completed") == 0) {
        signal = dbus_message_new_signal(OBJECT_PATH, INTERFACE_NAME, "Completed");
        logInfo("Creating Completed signal for UPDATE-AGENT");
    } else if (strcmp(member, "Progress") == 0) {
        signal = dbus_message_new_signal(OBJECT_PATH, INTERFACE_NAME, "Progress");
        logInfo("Creating Progress signal for UPDATE-AGENT");
    } else {
        logDebug("Unknown RAUC signal member, not forwarding: " + std::string(member));
        return;
    }

    if (signal) {
        // Copy and convert arguments from original message - EXACTLY like the working version
        DBusMessageIter src_iter, dst_iter;
        if (dbus_message_iter_init(message, &src_iter)) {
            dbus_message_iter_init_append(signal, &dst_iter);

            if (strcmp(member, "Completed") == 0) {
                // RAUC Completed: (bs) -> Our Completed: (bs) - EXACTLY like working version
                bool success = false;
                std::string message_text;

                // Debug: Log the actual argument type we received
                int arg_type = dbus_message_iter_get_arg_type(&src_iter);
                logInfo("Completed signal - Received argument type: " + std::to_string(arg_type));
                logInfo("Completed signal - DBUS_TYPE_BOOLEAN is: " + std::to_string(DBUS_TYPE_BOOLEAN));
                logInfo("Completed signal - DBUS_TYPE_INT32 is: " + std::to_string(DBUS_TYPE_INT32));

                // Parse boolean success - EXACTLY like working version
                if (dbus_message_iter_get_arg_type(&src_iter) == DBUS_TYPE_BOOLEAN) {
                    dbus_bool_t result;
                    dbus_message_iter_get_basic(&src_iter, &result);
                    success = result;
                    dbus_message_iter_append_basic(&dst_iter, DBUS_TYPE_BOOLEAN, &result);

                    logInfo("Completed signal - success: " + std::string(success ? "true" : "false"));
                } else {
                    logError("Completed signal: Expected boolean argument, got different type");
                    dbus_message_unref(signal);
                    return;
                }

                // Parse message string - EXACTLY like working version
                if (dbus_message_iter_next(&src_iter)) {
                    if (dbus_message_iter_get_arg_type(&src_iter) == DBUS_TYPE_STRING) {
                        const char* msg;
                        dbus_message_iter_get_basic(&src_iter, &msg);
                        message_text = msg ? msg : "";
                        dbus_message_iter_append_basic(&dst_iter, DBUS_TYPE_STRING, &msg);

                        logInfo("Completed signal - message: " + message_text);
                    } else {
                        logError("Completed signal: Expected string argument, got different type");
                        dbus_message_unref(signal);
                        return;
                    }
                } else {
                    logError("Completed signal: Missing second argument (message)");
                    dbus_message_unref(signal);
                    return;
                }

                logInfo("Completed signal parsed successfully: success=" + std::string(success ? "true" : "false") + ", message='" + message_text + "'");

            } else if (strcmp(member, "Progress") == 0) {
                // RAUC Progress: (i) -> Our Progress: (i) - EXACTLY like working version
                int arg_type = dbus_message_iter_get_arg_type(&src_iter);
                logInfo("Progress signal - Received argument type: " + std::to_string(arg_type));
                logInfo("Progress signal - DBUS_TYPE_INT32 is: " + std::to_string(DBUS_TYPE_INT32));

                if (dbus_message_iter_get_arg_type(&src_iter) == DBUS_TYPE_INT32) {
                    dbus_int32_t percentage;
                    dbus_message_iter_get_basic(&src_iter, &percentage);
                    dbus_message_iter_append_basic(&dst_iter, DBUS_TYPE_INT32, &percentage);

                    logInfo("Progress signal - percentage: " + std::to_string(percentage));
                } else {
                    logError("Progress signal: Expected integer argument, got different type");
                    dbus_message_unref(signal);
                    return;
                }
            }
        } else {
            logError("Failed to get signal arguments");
            dbus_message_unref(signal);
            return;
        }

        // Send the signal
        dbus_bool_t sent = dbus_connection_send(service_connection_, signal, nullptr);
        dbus_message_unref(signal);

        if (sent) {
            logInfo("Signal successfully forwarded to update-agent: " + std::string(member));
        } else {
            logError("Failed to send signal to update-agent: " + std::string(member));
        }
    } else {
        logError("Failed to create signal for forwarding: " + std::string(member ? member : "unknown"));
    }
}

// Method implementations - forward to RAUC with original method names
DBusMessage* UpdateService::handleInstall(DBusMessage* message) {
    logInfo("Install called - forwarding to RAUC Install");
    return forwardToRauc("Install", message);
}

DBusMessage* UpdateService::handleInstallBundle(DBusMessage* message) {
    logInfo("InstallBundle called - forwarding to RAUC InstallBundle");
    return forwardToRauc("InstallBundle", message);
}

DBusMessage* UpdateService::handleInfo(DBusMessage* message) {
    logInfo("Info called - forwarding to RAUC Info");
    return forwardToRauc("Info", message);
}

DBusMessage* UpdateService::handleInspectBundle(DBusMessage* message) {
    logInfo("InspectBundle called - forwarding to RAUC InspectBundle");
    return forwardToRauc("InspectBundle", message);
}

DBusMessage* UpdateService::handleMark(DBusMessage* message) {
    logInfo("Mark called - forwarding to RAUC Mark");
    return forwardToRauc("Mark", message);
}

DBusMessage* UpdateService::handleGetSlotStatus(DBusMessage* message) {
    logInfo("GetSlotStatus called - forwarding to RAUC GetSlotStatus");
    return forwardToRauc("GetSlotStatus", message);
}

DBusMessage* UpdateService::handleGetArtifactStatus(DBusMessage* message) {
    logInfo("GetArtifactStatus called - forwarding to RAUC GetArtifactStatus");
    return forwardToRauc("GetArtifactStatus", message);
}

DBusMessage* UpdateService::handleGetPrimary(DBusMessage* message) {
    logInfo("GetPrimary called - forwarding to RAUC GetPrimary");
    return forwardToRauc("GetPrimary", message);
}

// Property implementations - forward to RAUC properties
DBusMessage* UpdateService::handleGetOperation(DBusMessage* message) {
    logDebug("Getting Operation property");
    DBusMessage* reply = getRaucProperty("Operation", message);
    if (!reply) {
        logError("Failed to get Operation property from RAUC");
        return createErrorReply(message, "org.freedesktop.UpdateService.Error", "Failed to get Operation property");
    }
    return reply;
}

DBusMessage* UpdateService::handleGetLastError(DBusMessage* message) {
    logDebug("Getting LastError property");
    DBusMessage* reply = getRaucProperty("LastError", message);
    if (!reply) {
        logError("Failed to get LastError property from RAUC");
        return createErrorReply(message, "org.freedesktop.UpdateService.Error", "Failed to get LastError property");
    }
    return reply;
}

DBusMessage* UpdateService::handleGetProgress(DBusMessage* message) {
    logDebug("Getting Progress property");
    DBusMessage* reply = getRaucProperty("Progress", message);
    if (!reply) {
        logError("Failed to get Progress property from RAUC");
        return createErrorReply(message, "org.freedesktop.UpdateService.Error", "Failed to get Progress property");
    }
    return reply;
}

DBusMessage* UpdateService::handleGetCompatible(DBusMessage* message) {
    logDebug("Getting Compatible property");
    DBusMessage* reply = getRaucProperty("Compatible", message);
    if (!reply) {
        logError("Failed to get Compatible property from RAUC");
        return createErrorReply(message, "org.freedesktop.UpdateService.Error", "Failed to get Compatible property");
    }
    return reply;
}

DBusMessage* UpdateService::handleGetVariant(DBusMessage* message) {
    logDebug("Getting Variant property");
    DBusMessage* reply = getRaucProperty("Variant", message);
    if (!reply) {
        logError("Failed to get Variant property from RAUC");
        return createErrorReply(message, "org.freedesktop.UpdateService.Error", "Failed to get Variant property");
    }
    return reply;
}

DBusMessage* UpdateService::handleGetBootSlot(DBusMessage* message) {
    logDebug("Getting BootSlot property");
    DBusMessage* reply = getRaucProperty("BootSlot", message);
    if (!reply) {
        logError("Failed to get BootSlot property from RAUC");
        return createErrorReply(message, "org.freedesktop.UpdateService.Error", "Failed to get BootSlot property");
    }
    return reply;
}

DBusMessage* UpdateService::forwardToRauc(const std::string& rauc_method_name, DBusMessage* message) {
    logDebug("Forwarding to RAUC: " + rauc_method_name);

    if (!connected_to_rauc_) {
        logError("Not connected to RAUC service");
        return createErrorReply(message, "de.makepluscode.updateservice.Error", "Not connected to RAUC");
    }

    // Create method call to RAUC
    DBusMessage* rauc_call = dbus_message_new_method_call(
        RAUC_SERVICE_NAME,
        RAUC_OBJECT_PATH,
        RAUC_INTERFACE_NAME,
        rauc_method_name.c_str()
    );

    if (!rauc_call) {
        logError("Failed to create D-Bus method call for RAUC");
        return createErrorReply(message, "de.makepluscode.updateservice.Error", "Failed to create RAUC call");
    }

    logDebug("Created RAUC method call: " + std::string(RAUC_SERVICE_NAME) + "." + rauc_method_name);

    // Copy arguments from original message
    DBusMessageIter src_iter, dst_iter;
    if (dbus_message_iter_init(message, &src_iter)) {
        dbus_message_iter_init_append(rauc_call, &dst_iter);

        // Copy all arguments
        do {
            int arg_type = dbus_message_iter_get_arg_type(&src_iter);
            if (arg_type == DBUS_TYPE_INVALID) break;

            // Copy basic types and variants
            if (arg_type == DBUS_TYPE_STRING) {
                const char* value;
                dbus_message_iter_get_basic(&src_iter, &value);
                dbus_message_iter_append_basic(&dst_iter, DBUS_TYPE_STRING, &value);
            }
            // Add more argument copying as needed for complex types

        } while (dbus_message_iter_next(&src_iter));
    }

    // Send call to RAUC and wait for reply

    DBusMessage* rauc_reply = dbus_connection_send_with_reply_and_block(
        rauc_connection_, rauc_call, 30000, nullptr);

    dbus_message_unref(rauc_call);

    if (!rauc_reply) {
        logError("RAUC method call failed: " + rauc_method_name);
        return createErrorReply(message, "de.makepluscode.updateservice.Error", "RAUC call failed");
    }

    logDebug("RAUC method completed: " + rauc_method_name);

    // Create reply with same content as RAUC reply
    DBusMessage* reply = dbus_message_new_method_return(message);
    if (reply) {
        // Copy reply arguments
        DBusMessageIter reply_src_iter, reply_dst_iter;
        if (dbus_message_iter_init(rauc_reply, &reply_src_iter)) {
            dbus_message_iter_init_append(reply, &reply_dst_iter);

            // Copy all reply arguments
            do {
                int arg_type = dbus_message_iter_get_arg_type(&reply_src_iter);
                if (arg_type == DBUS_TYPE_INVALID) break;

                if (arg_type == DBUS_TYPE_STRING) {
                    const char* value;
                    dbus_message_iter_get_basic(&reply_src_iter, &value);
                    dbus_message_iter_append_basic(&reply_dst_iter, DBUS_TYPE_STRING, &value);
                }
                // Add more types as needed

            } while (dbus_message_iter_next(&reply_src_iter));
        }
    }

    dbus_message_unref(rauc_reply);
    return reply;
}

DBusMessage* UpdateService::getRaucProperty(const std::string& property_name, DBusMessage* original_message) {
    logDebug("Getting RAUC property: " + property_name);

    if (!connected_to_rauc_) {
        logError("Not connected to RAUC service for property: " + property_name);
        return nullptr;
    }

    // Create property get call to RAUC
    DBusMessage* prop_call = dbus_message_new_method_call(
        RAUC_SERVICE_NAME,
        RAUC_OBJECT_PATH,
        RAUC_PROPERTIES_INTERFACE,
        "Get"
    );

    if (!prop_call) {
        logError("Failed to create property call message for: " + property_name);
        return nullptr;
    }

    // Add arguments (interface name and property name)
    DBusMessageIter iter;
    dbus_message_iter_init_append(prop_call, &iter);

    const char* interface_name = RAUC_INTERFACE_NAME;
    const char* prop_name = property_name.c_str();

    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface_name);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop_name);

    // Send call and get reply
    DBusMessage* rauc_reply = dbus_connection_send_with_reply_and_block(
        rauc_connection_, prop_call, 30000, nullptr);

    dbus_message_unref(prop_call);

    if (!rauc_reply) {
        logError("RAUC property call failed: " + property_name);
        return nullptr;
    }

    // Check if reply is an error
    if (dbus_message_get_type(rauc_reply) == DBUS_MESSAGE_TYPE_ERROR) {
        logError("RAUC returned error for property: " + property_name);
        const char* error_name = dbus_message_get_error_name(rauc_reply);
        logError("Error name: " + std::string(error_name ? error_name : "unknown"));
        dbus_message_unref(rauc_reply);
        return nullptr;
    }

    // Create a proper Properties.Get reply that wraps the RAUC property value in a variant
    DBusMessage* reply = dbus_message_new_method_return(original_message);
    if (!reply) {
        logError("Failed to create Properties.Get reply for: " + property_name);
        dbus_message_unref(rauc_reply);
        return nullptr;
    }

    // Parse the RAUC reply and wrap it in a variant
    DBusMessageIter rauc_iter, reply_iter, variant_iter;
    if (dbus_message_iter_init(rauc_reply, &rauc_iter)) {
        dbus_message_iter_init_append(reply, &reply_iter);

        // Start a variant
        const char* variant_signature = nullptr;
        int arg_type = dbus_message_iter_get_arg_type(&rauc_iter);

        if (arg_type == DBUS_TYPE_VARIANT) {
            // RAUC already returns a variant, so we can copy it directly
            DBusMessageIter rauc_variant_iter;
            dbus_message_iter_recurse(&rauc_iter, &rauc_variant_iter);
            variant_signature = dbus_message_iter_get_signature(&rauc_variant_iter);
        } else {
            // RAUC returns a direct value, we need to wrap it in a variant
            variant_signature = dbus_message_iter_get_signature(&rauc_iter);
        }

        if (variant_signature) {
            dbus_message_iter_open_container(&reply_iter, DBUS_TYPE_VARIANT, variant_signature, &variant_iter);

            if (arg_type == DBUS_TYPE_VARIANT) {
                // Copy the variant content directly by iterating through it
                DBusMessageIter rauc_variant_iter;
                dbus_message_iter_recurse(&rauc_iter, &rauc_variant_iter);

                // Copy the variant content manually
                int variant_arg_type = dbus_message_iter_get_arg_type(&rauc_variant_iter);
                if (variant_arg_type == DBUS_TYPE_STRING) {
                    const char* value;
                    dbus_message_iter_get_basic(&rauc_variant_iter, &value);
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
                } else if (variant_arg_type == DBUS_TYPE_INT32) {
                    dbus_int32_t value;
                    dbus_message_iter_get_basic(&rauc_variant_iter, &value);
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &value);
                } else if (variant_arg_type == DBUS_TYPE_BOOLEAN) {
                    dbus_bool_t value;
                    dbus_message_iter_get_basic(&rauc_variant_iter, &value);
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
                }
                // Add more types as needed
            } else {
                // Copy the direct value into our variant manually
                if (arg_type == DBUS_TYPE_STRING) {
                    const char* value;
                    dbus_message_iter_get_basic(&rauc_iter, &value);
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
                } else if (arg_type == DBUS_TYPE_INT32) {
                    dbus_int32_t value;
                    dbus_message_iter_get_basic(&rauc_iter, &value);
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &value);
                } else if (arg_type == DBUS_TYPE_BOOLEAN) {
                    dbus_bool_t value;
                    dbus_message_iter_get_basic(&rauc_iter, &value);
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
                }
                // Add more types as needed
            }

            dbus_message_iter_close_container(&reply_iter, &variant_iter);
        } else {
            logError("Failed to get variant signature for property: " + property_name);
            dbus_message_unref(reply);
            dbus_message_unref(rauc_reply);
            return nullptr;
        }
    } else {
        logError("Failed to parse RAUC reply for property: " + property_name);
        dbus_message_unref(reply);
        dbus_message_unref(rauc_reply);
        return nullptr;
    }

    dbus_message_unref(rauc_reply);
    logDebug("Successfully created Properties.Get reply for: " + property_name);
    return reply;
}

DBusMessage* UpdateService::createErrorReply(DBusMessage* message,
                                           const std::string& error_name,
                                           const std::string& error_message) {
    DBusMessage* reply = dbus_message_new_error(message, error_name.c_str(), error_message.c_str());
    logError("Returning error: " + error_name + " - " + error_message);
    return reply;
}

void UpdateService::logInfo(const std::string& message) {
    DLT_LOG(dlt_context_service, DLT_LOG_INFO, DLT_STRING(message.c_str()));
}

void UpdateService::logError(const std::string& message) {
    DLT_LOG(dlt_context_service, DLT_LOG_ERROR, DLT_STRING(message.c_str()));
}

void UpdateService::logDebug(const std::string& message) {
    DLT_LOG(dlt_context_service, DLT_LOG_DEBUG, DLT_STRING(message.c_str()));
}
