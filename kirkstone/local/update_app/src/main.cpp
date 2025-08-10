#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <dlt/dlt.h>

DLT_DECLARE_CONTEXT(updateContext);

volatile bool running = true;

void signalHandler(int signal) {
    running = false;
}

int main() {
    // Initialize DLT
    DLT_REGISTER_APP("UAPP", "Update Application");
    DLT_REGISTER_CONTEXT(updateContext, "MAIN", "Main context for update app");
    
    // Register signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update application started"));
    
    while (running) {
        // Print hello message
        std::cout << "hello, update" << std::endl;
        
        // DLT sample log
        DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update sample"));
        
        // Wait for 2 seconds (different interval than service-app)
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    DLT_LOG(updateContext, DLT_LOG_INFO, DLT_STRING("Update application stopping"));
    
    // Unregister DLT
    DLT_UNREGISTER_CONTEXT(updateContext);
    DLT_UNREGISTER_APP();
    
    return 0;
}