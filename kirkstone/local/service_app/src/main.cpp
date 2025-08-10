#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <dlt/dlt.h>

DLT_DECLARE_CONTEXT(serviceContext);

volatile bool running = true;

void signalHandler(int signal) {
    running = false;
}

int main() {
    // Initialize DLT
    DLT_REGISTER_APP("SAPP", "Service Application");
    DLT_REGISTER_CONTEXT(serviceContext, "MAIN", "Main context for service app");
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    DLT_LOG(serviceContext, DLT_LOG_INFO, DLT_STRING("Service application started"));
    
    while (running) {
        // Print hello message
        std::cout << "hello, service" << std::endl;
        
        // DLT heartbeat log
        DLT_LOG(serviceContext, DLT_LOG_INFO, DLT_STRING("Heartbeat"));
        
        // Wait for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    DLT_LOG(serviceContext, DLT_LOG_INFO, DLT_STRING("Service application stopping"));
    
    // Unregister DLT
    DLT_UNREGISTER_CONTEXT(serviceContext);
    DLT_UNREGISTER_APP();
    
    return 0;
}