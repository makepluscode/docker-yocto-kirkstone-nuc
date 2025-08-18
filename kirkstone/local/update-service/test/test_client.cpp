#include <iostream>
#include <dbus/dbus.h>
#include <cstring>

/**
 * Simple test client for update-service D-Bus broker
 * Tests basic connectivity and method forwarding
 */

bool testUpdateServiceConnection() {
    std::cout << "Testing connection to update-service..." << std::endl;

    DBusError error;
    dbus_error_init(&error);

    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        std::cerr << "❌ Failed to connect to D-Bus: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }

    // Test if update-service is available
    if (!dbus_bus_name_has_owner(connection, "org.freedesktop.UpdateService", &error)) {
        std::cerr << "❌ Update service is not available: " << error.message << std::endl;
        dbus_error_free(&error);
        dbus_connection_unref(connection);
        return false;
    }

    std::cout << "✅ Update service is available" << std::endl;

    // Test calling GetSlotStatus method
    std::cout << "Testing GetSlotStatus method..." << std::endl;

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
        "org.freedesktop.UpdateService",
        "GetSlotStatus"
    );

    if (!message) {
        std::cerr << "❌ Failed to create D-Bus message" << std::endl;
        dbus_connection_unref(connection);
        return false;
    }

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, message, 10000, &error);
    dbus_message_unref(message);

    if (dbus_error_is_set(&error)) {
        std::cerr << "❌ D-Bus call failed: " << error.message << std::endl;
        dbus_error_free(&error);
        dbus_connection_unref(connection);
        return false;
    }

    if (!reply) {
        std::cerr << "❌ No reply received" << std::endl;
        dbus_connection_unref(connection);
        return false;
    }

    std::cout << "✅ GetSlotStatus call successful" << std::endl;

    dbus_message_unref(reply);
    dbus_connection_unref(connection);

    return true;
}

bool testUpdateServiceProperty() {
    std::cout << "Testing Operation property..." << std::endl;

    DBusError error;
    dbus_error_init(&error);

    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error)) {
        std::cerr << "❌ Failed to connect to D-Bus: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }

    DBusMessage* message = dbus_message_new_method_call(
        "org.freedesktop.UpdateService",
        "/org/freedesktop/UpdateService",
        "org.freedesktop.DBus.Properties",
        "Get"
    );

    if (!message) {
        std::cerr << "❌ Failed to create D-Bus message" << std::endl;
        dbus_connection_unref(connection);
        return false;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);

    const char* interface_name = "org.freedesktop.UpdateService";
    const char* property_name = "Operation";

    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface_name);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &property_name);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, message, 10000, &error);
    dbus_message_unref(message);

    if (dbus_error_is_set(&error)) {
        std::cerr << "❌ Property get failed: " << error.message << std::endl;
        dbus_error_free(&error);
        dbus_connection_unref(connection);
        return false;
    }

    if (reply) {
        std::cout << "✅ Operation property call successful" << std::endl;
        dbus_message_unref(reply);
    }

    dbus_connection_unref(connection);
    return true;
}

int main() {
    std::cout << "=== Update Service D-Bus Broker Test ===" << std::endl;
    std::cout << "Testing connection and basic functionality" << std::endl;
    std::cout << std::endl;

    bool connection_ok = testUpdateServiceConnection();
    bool property_ok = testUpdateServiceProperty();

    std::cout << std::endl;
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Connection test: " << (connection_ok ? "✅ PASS" : "❌ FAIL") << std::endl;
    std::cout << "Property test: " << (property_ok ? "✅ PASS" : "❌ FAIL") << std::endl;

    if (connection_ok && property_ok) {
        std::cout << std::endl;
        std::cout << "🎉 All tests passed! Update service broker is working." << std::endl;
        return 0;
    } else {
        std::cout << std::endl;
        std::cout << "❌ Some tests failed. Check service status and logs." << std::endl;
        return 1;
    }
}
