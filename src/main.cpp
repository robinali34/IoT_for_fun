#include "device_manager.h"
#include "database.h"
#include "api_server.h"
#include <iostream>
#include <signal.h>
#include <memory>
#include <thread>
#include <chrono>

// Global variables for signal handling
std::shared_ptr<APIServer> g_apiServer;
std::shared_ptr<DeviceManager> g_deviceManager;
std::shared_ptr<Database> g_database;
bool g_running = true;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false;
    
    if (g_apiServer) {
        g_apiServer->stop();
    }
    
    if (g_deviceManager) {
        g_deviceManager->stopMonitoring();
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -p, --port PORT        API server port (default: 8080)" << std::endl;
    std::cout << "  -d, --database PATH    Database file path (default: tplink_devices.db)" << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << "  -v, --verbose          Enable verbose logging" << std::endl;
    std::cout << "  --discover-only        Only discover devices and exit" << std::endl;
    std::cout << "  --no-monitoring        Disable device monitoring" << std::endl;
}

void printBanner() {
    std::cout << "========================================" << std::endl;
    std::cout << "    TP-Link Device Controller v1.0" << std::endl;
    std::cout << "    Raspberry Pi 4 Compatible" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Default configuration
    int port = 8080;
    std::string dbPath = "tplink_devices.db";
    bool verbose = false;
    bool discoverOnly = false;
    bool enableMonitoring = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                port = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --port requires a value" << std::endl;
                return 1;
            }
        } else if (arg == "-d" || arg == "--database") {
            if (i + 1 < argc) {
                dbPath = argv[++i];
            } else {
                std::cerr << "Error: --database requires a value" << std::endl;
                return 1;
            }
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--discover-only") {
            discoverOnly = true;
        } else if (arg == "--no-monitoring") {
            enableMonitoring = false;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize database
        std::cout << "Initializing database..." << std::endl;
        g_database = std::make_shared<Database>(dbPath);
        if (!g_database->initialize()) {
            std::cerr << "Failed to initialize database" << std::endl;
            return 1;
        }
        std::cout << "Database initialized successfully" << std::endl;
        
        // Initialize device manager
        std::cout << "Initializing device manager..." << std::endl;
        g_deviceManager = std::make_shared<DeviceManager>();
        
        // Load existing devices from database
        auto existingDevices = g_database->getAllDevices();
        std::cout << "Found " << existingDevices.size() << " existing devices in database" << std::endl;
        
        for (const auto& device : existingDevices) {
            if (verbose) {
                std::cout << "  - " << device.name << " (" << device.ip << ") - " 
                         << (device.isOnline ? "Online" : "Offline") << std::endl;
            }
            g_deviceManager->addDevice(device.ip, device.port);
        }
        
        // Discover new devices
        std::cout << "Discovering TP-Link devices..." << std::endl;
        auto discoveredDevices = g_deviceManager->discoverDevices();
        std::cout << "Discovered " << discoveredDevices.size() << " devices" << std::endl;
        
        if (verbose) {
            for (const auto& device : discoveredDevices) {
                std::cout << "  - " << device.name << " (" << device.ip << ") - " 
                         << device.model << " - " << (device.isOnline ? "Online" : "Offline") << std::endl;
            }
        }
        
        if (discoverOnly) {
            std::cout << "Discovery complete. Exiting." << std::endl;
            return 0;
        }
        
        // Start device monitoring
        if (enableMonitoring) {
            std::cout << "Starting device monitoring..." << std::endl;
            g_deviceManager->startMonitoring();
        }
        
        // Initialize and start API server
        std::cout << "Starting API server on port " << port << "..." << std::endl;
        g_apiServer = std::make_shared<APIServer>(port);
        g_apiServer->setDeviceManager(g_deviceManager);
        g_apiServer->setDatabase(g_database);
        
        if (!g_apiServer->start()) {
            std::cerr << "Failed to start API server" << std::endl;
            return 1;
        }
        
        std::cout << "API server started successfully!" << std::endl;
        std::cout << "API endpoints available at:" << std::endl;
        std::cout << "  GET  http://localhost:" << port << "/health" << std::endl;
        std::cout << "  POST http://localhost:" << port << "/api/discover" << std::endl;
        std::cout << "  GET  http://localhost:" << port << "/api/devices" << std::endl;
        std::cout << "  GET  http://localhost:" << port << "/api/devices/{deviceId}" << std::endl;
        std::cout << "  POST http://localhost:" << port << "/api/devices/{deviceId}/power" << std::endl;
        std::cout << "  POST http://localhost:" << port << "/api/devices/{deviceId}/brightness" << std::endl;
        std::cout << "  POST http://localhost:" << port << "/api/devices/{deviceId}/color" << std::endl;
        std::cout << "  POST http://localhost:" << port << "/api/devices/{deviceId}/colortemp" << std::endl;
        std::cout << "  GET  http://localhost:" << port << "/api/stats" << std::endl;
        std::cout << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        // Main loop
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Shutting down..." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
