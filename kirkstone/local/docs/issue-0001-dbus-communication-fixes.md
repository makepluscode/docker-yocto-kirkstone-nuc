# Issue #0001: D-Bus Communication Fixes Between Update-Agent and Update-Service

## Problem Description

The D-Bus communication between `update-agent` (client) and `update-service` (broker) was not working properly. While direct communication with RAUC worked, the broker-based communication failed, preventing progress feedback and proper update handling.

## Root Cause Analysis

The issue was caused by multiple problems in the D-Bus communication layer:

1. **Method name mismatches** between client expectations and service interface definitions
2. **Missing method parameters** for required D-Bus calls
3. **Incorrect property reply format** handling
4. **Signal signature mismatches** between RAUC's actual signals and expected formats
5. **Overly broad signal filtering** causing incorrect signal processing
6. **Incorrect signal argument parsing** and forwarding

## Files Modified

### 1. `update-agent/src/service_agent.cpp`

#### Changes Made:

**Method Name Fix:**
```cpp
// Changed from "GetBootSlot" to "GetPrimary" to match interface definition
DBusMessage* message = dbus_message_new_method_call(
    "org.freedesktop.UpdateService",
    "/org/freedesktop/UpdateService",
    "org.freedesktop.UpdateService",
    "GetPrimary" // Changed from GetBootSlot
);
```

**Method Parameter Fixes:**
```cpp
// Added required parameters for Mark method calls
DBusMessage* message = dbus_message_new_method_call(
    "org.freedesktop.UpdateService",
    "/org/freedesktop/UpdateService",
    "org.freedesktop.UpdateService",
    "Mark"
);
DBusMessageIter iter;
dbus_message_iter_init_append(message, &iter);
const char* state = "good"; // or "bad" for markBad()
dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &state);
const char* slot_identifier = "booted";
dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &slot_identifier);
```

**Enhanced Signal Logging:**
```cpp
// Added detailed signal logging for debugging
DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("=== SIGNAL RECEIVED ==="));
DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Interface: "), DLT_STRING(interface ? interface : "null"));
DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Member: "), DLT_STRING(member ? member : "null"));
DLT_LOG(dlt_context_updater, DLT_LOG_INFO, DLT_STRING("Sender: "), DLT_STRING(sender ? sender : "null"));
```

**Improved Message Processing:**
```cpp
// Added logging for D-Bus message processing
int dispatch_count = 0;
while (dbus_connection_get_dispatch_status(connection_) == DBUS_DISPATCH_DATA_REMAINS) {
    dbus_connection_dispatch(connection_);
    dispatch_count++;
}
if (dispatch_count > 0) {
    DLT_LOG(dlt_context_updater, DLT_LOG_DEBUG, DLT_STRING("Processed "), DLT_INT(dispatch_count), DLT_STRING(" D-Bus messages"));
}
```

### 2. `update-service/src/update_service.cpp`

#### Changes Made:

**Property Reply Format Fix:**
```cpp
// Modified getRaucProperty to create proper Properties.Get reply format
DBusMessage* getRaucProperty(const std::string& property_name, DBusMessage* original_message) {
    // ... RAUC call logic ...
    DBusMessage* reply = dbus_message_new_method_return(original_message);
    // Wrap RAUC property value in variant and append to reply
    // ... variant handling logic ...
    return reply;
}
```

**Signal Filtering Fix:**
```cpp
// Reverted to specific signal filtering matching old working version
dbus_bus_add_match(rauc_connection_, "type='signal',interface='de.pengutronix.rauc.Installer'", &error);
```

**Enhanced RAUC Signal Logging:**
```cpp
// Added detailed logging for incoming RAUC signals
service->logInfo("=== RAUC SIGNAL RECEIVED ===");
service->logInfo("Interface: " + std::string(interface ? interface : "null"));
service->logInfo("Member: " + std::string(member ? member : "null"));
service->logInfo("Sender: " + std::string(sender ? sender : "null"));
```

**Signal Forwarding Fix:**
```cpp
// Fixed signal argument copying based on actual RAUC signal signatures
if (strcmp(member, "Completed") == 0) {
    // Copy boolean success (RAUC sends (bs) not (i))
    if (dbus_message_iter_get_arg_type(&src_iter) == DBUS_TYPE_BOOLEAN) {
        dbus_bool_t success;
        dbus_message_iter_get_basic(&src_iter, &success);
        dbus_message_iter_append_basic(&dest_iter, DBUS_TYPE_BOOLEAN, &success);
    } else {
        logError("Expected boolean");
        dbus_message_unref(signal);
        return;
    }
    // Copy message string
    if (dbus_message_iter_next(&src_iter)) {
        if (dbus_message_iter_get_arg_type(&src_iter) == DBUS_TYPE_STRING) {
            const char* message;
            dbus_message_iter_get_basic(&src_iter, &message);
            dbus_message_iter_append_basic(&dest_iter, DBUS_TYPE_STRING, &message);
        } else {
            logError("Expected string");
            dbus_message_unref(signal);
            return;
        }
    } else {
        logError("Missing message");
        dbus_message_unref(signal);
        return;
    }
} else if (strcmp(member, "Progress") == 0) {
    // Copy percentage integer (RAUC sends (i) not (isi))
    if (dbus_message_iter_get_arg_type(&src_iter) == DBUS_TYPE_INT32) {
        dbus_int32_t percentage;
        dbus_message_iter_get_basic(&src_iter, &percentage);
        dbus_message_iter_append_basic(&dest_iter, DBUS_TYPE_INT32, &percentage);
    } else {
        logError("Expected integer");
        dbus_message_unref(signal);
        return;
    }
}
```

**Improved RAUC Message Processing:**
```cpp
// Enhanced RAUC message dispatching in main loop
int rauc_dispatch_count = 0;
while (dbus_connection_get_dispatch_status(rauc_connection_) == DBUS_DISPATCH_DATA_REMAINS) {
    dbus_connection_dispatch(rauc_connection_);
    rauc_dispatch_count++;
}
dbus_connection_read_write(rauc_connection_, 10);
if (rauc_dispatch_count > 0 && loop_count % 100 == 0) {
    logInfo("RAUC messages processed: " + std::to_string(rauc_dispatch_count));
}
```

### 3. `update-service/src/update_service.h`

#### Changes Made:

**Method Signature Update:**
```cpp
// Updated getRaucProperty signature to include original message
DBusMessage* getRaucProperty(const std::string& property_name, DBusMessage* original_message);
```

### 4. `update-service/src/org.freedesktop.UpdateService.xml`

#### Changes Made:

**Signal Signature Corrections:**
```xml
<!-- Fixed Completed signal signature from (i) to (bs) -->
<signal name="Completed">
    <arg name="success" type="b"/>
    <arg name="message" type="s"/>
</signal>

<!-- Fixed Progress signal signature from (isi) to (i) -->
<signal name="Progress">
    <arg name="percentage" type="i"/>
</signal>
```

## Error Resolution Process

### Error 1: `dbus_message_iter_copy` Not Declared
- **Issue**: Used non-existent D-Bus API function
- **Fix**: Refactored to manually iterate and copy message arguments

### Error 2: Property Reply Format Mismatch
- **Issue**: `update-service` returning raw RAUC reply instead of proper `Properties.Get` format
- **Fix**: Modified `getRaucProperty` to wrap RAUC values in D-Bus variants

### Error 3: Method Name Mismatch
- **Issue**: Client calling `GetBootSlot` but interface defined `GetPrimary`
- **Fix**: Updated method call to use correct name

### Error 4: Missing Method Parameters
- **Issue**: `Mark` method calls missing required `state` and `slot_identifier` parameters
- **Fix**: Added explicit parameter construction in `markGood()` and `markBad()`

### Error 5: Signal Signature Mismatch
- **Issue**: Initial assumption that RAUC signals were `(i)` and `(isi)` instead of actual `(bs)` and `(i)`
- **Fix**: Updated XML interface definition and simplified signal forwarding logic

### Error 6: Overly Broad Signal Filtering
- **Issue**: Service receiving non-RAUC signals like `PropertiesChanged`
- **Fix**: Reverted to specific `de.pengutronix.rauc.Installer` interface filtering

### Error 7: Signal Argument Parsing Failure
- **Issue**: `Completed` signal arguments failing to parse in agent
- **Fix**: Enhanced argument extraction and error handling in `forwardRaucSignal`

## Testing and Verification

### Log Analysis Process
1. **Initial Diagnosis**: Used `dlt-receive.sh` to monitor D-Bus communication
2. **Method Call Verification**: Confirmed `Properties.Get` and `Install` calls working
3. **Signal Troubleshooting**: Identified missing progress feedback
4. **Signal Filtering**: Discovered overly broad filtering causing issues
5. **Signal Parsing**: Fixed argument extraction and forwarding
6. **Final Verification**: Confirmed successful bundle installation with proper signal handling

### Key Log Entries
- **Successful Properties.Get**: `=== Properties.Get call succeeded ===`
- **Successful Install**: `Method call successful: Install`
- **RAUC Signal Reception**: `=== RAUC SIGNAL RECEIVED ===`
- **Signal Forwarding**: `=== FORWARDING RAUC SIGNAL ===`
- **Bundle Installation**: `Bundle installation started successfully`

## Results

After implementing all fixes:

1. ✅ **D-Bus Method Calls**: All method calls (`GetPrimary`, `Mark`, `Install`, `Properties.Get`) working correctly
2. ✅ **Property Handling**: Proper `Properties.Get` reply format with variant wrapping
3. ✅ **Signal Forwarding**: RAUC signals (`Progress`, `Completed`) correctly forwarded to agent
4. ✅ **Bundle Installation**: Successful bundle download and installation initiation
5. ✅ **Progress Feedback**: Progress signals received during installation process
6. ✅ **Error Handling**: Robust error checking and logging throughout the process

## Lessons Learned

1. **D-Bus Interface Consistency**: Method names and signatures must match exactly between client and service
2. **Signal Signature Verification**: Always verify actual signal signatures from source (RAUC) rather than assuming
3. **Property Reply Format**: `Properties.Get` calls require specific reply format with variant wrapping
4. **Signal Filtering**: Use specific interface filters to avoid processing unrelated signals
5. **Error Logging**: Comprehensive logging is essential for debugging D-Bus communication issues
6. **Message Processing**: Proper D-Bus message dispatching is crucial for signal handling

## Files Status

- ✅ `update-agent/src/service_agent.cpp` - Fixed method calls and signal handling
- ✅ `update-service/src/update_service.cpp` - Fixed property handling and signal forwarding
- ✅ `update-service/src/update_service.h` - Updated method signatures
- ✅ `update-service/src/org.freedesktop.UpdateService.xml` - Corrected signal signatures

## Next Steps

1. Monitor ongoing update processes to ensure stability
2. Consider adding unit tests for D-Bus communication
3. Document D-Bus interface specifications for future reference
4. Implement additional error recovery mechanisms if needed

---

**Issue Status**: ✅ RESOLVED  
**Date**: 2025-08-17  
**Resolution**: All D-Bus communication issues fixed, update process working correctly 