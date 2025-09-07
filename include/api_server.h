#pragma once

#include "device_manager.h"
#include "database.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>

class APIServer {
public:
    APIServer(int port = 8080);
    ~APIServer();

    // Server control
    bool start();
    void stop();
    bool isRunning();
    
    // Configuration
    void setPort(int port);
    void setDeviceManager(std::shared_ptr<DeviceManager> deviceManager);
    void setDatabase(std::shared_ptr<Database> database);
    
private:
    void setupRoutes();
    void runServer();
    
    int port_;
    std::shared_ptr<DeviceManager> deviceManager_;
    std::shared_ptr<Database> database_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> should_stop_;
    
    // HTTP server instance (will be httplib::Server*)
    void* server_;
};
