#pragma once

#include <string>
#include <vector>
#include "tplink_device.h"

class Database {
public:
    Database(const std::string& dbPath = "tplink_devices.db");
    ~Database();

    // Database initialization
    bool initialize();
    bool isOpen();
    
    // Device management
    bool addDevice(const DeviceInfo& device);
    bool updateDevice(const DeviceInfo& device);
    bool removeDevice(const std::string& deviceId);
    DeviceInfo getDevice(const std::string& deviceId);
    std::vector<DeviceInfo> getAllDevices();
    std::vector<DeviceInfo> getDevicesByStatus(bool isOnline);
    
    // Device status updates
    bool updateDeviceStatus(const std::string& deviceId, bool isOnline);
    bool updateDeviceState(const std::string& deviceId, bool isOn, int brightness = -1, 
                          int colorTemp = -1, int hue = -1, int saturation = -1);
    
    // Device discovery history
    bool addDiscoveryRecord(const std::string& ip, const std::string& deviceId, 
                           const std::string& model, bool success);
    std::vector<std::string> getKnownIPs();
    
    // Statistics
    int getDeviceCount();
    int getOnlineDeviceCount();
    int getOfflineDeviceCount();
    
private:
    std::string dbPath_;
    void* db_; // sqlite3* (using void* to avoid including sqlite3.h in header)
    
    bool executeQuery(const std::string& query);
    bool executeQueryWithResult(const std::string& query, 
                               std::function<int(void*, int, char**, char**)> callback,
                               void* data = nullptr);
};
