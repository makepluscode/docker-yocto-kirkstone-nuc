#include <iostream>
#include <signal.h>
#include <dlt/dlt.h>
#include "update_service.h"

DLT_DECLARE_CONTEXT(mainContext);

static UpdateService* g_service = nullptr;

void signalHandler(int signal) {
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Received signal "), DLT_INT(signal), DLT_STRING(", shutting down..."));
    if (g_service) {
        g_service->stop();
    }
}

int main() {
    // Initialize DLT
    DLT_REGISTER_APP("USVC", "Update Service Broker");
    DLT_REGISTER_CONTEXT(mainContext, "MAIN", "Main context for update service");
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("=== Update Service Broker Starting ==="));
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Service: org.freedesktop.UpdateService"));
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Interface: org.freedesktop.UpdateService"));
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Purpose: D-Bus broker between update-agent and RAUC"));
    
    // Create and initialize update service
    UpdateService service;
    g_service = &service;
    
    if (!service.initialize()) {
        DLT_LOG(mainContext, DLT_LOG_FATAL, DLT_STRING("Failed to initialize Update Service"));
        DLT_UNREGISTER_CONTEXT(mainContext);
        DLT_UNREGISTER_APP();
        return 1;
    }
    
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Update Service initialized successfully"));
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Ready to broker calls between update-agent and RAUC"));
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("Starting main service loop..."));
    
    // Run service
    service.run();
    
    DLT_LOG(mainContext, DLT_LOG_INFO, DLT_STRING("=== Update Service Broker Stopped ==="));
    
    // Cleanup
    g_service = nullptr;
    DLT_UNREGISTER_CONTEXT(mainContext);
    DLT_UNREGISTER_APP();
    
    return 0;
}