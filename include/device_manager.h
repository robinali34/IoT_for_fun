#pragma once

#include "tplink_device.h"
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    // Device discovery and management
    std::vector<DeviceInfo> discoverDevices();
    bool addDevice(const std::string& ip, int port = 9999);
    bool removeDevice(const std::string& deviceId);
    std::vector<DeviceInfo> getAllDevices();
    std::shared_ptr<TPLinkDevice> getDevice(const std::string& deviceId);
    
    // Device control
    bool turnOnDevice(const std::string& deviceId);
    bool turnOffDevice(const std::string& deviceId);
    bool toggleDevice(const std::string& deviceId);
    bool setDeviceBrightness(const std::string& deviceId, int brightness);
    bool setDeviceColor(const std::string& deviceId, int hue, int saturation, int value);
    bool setDeviceColorTemp(const std::string& deviceId, int temp);
    
    // Status monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring();
    
    // Device information
    DeviceInfo getDeviceInfo(const std::string& deviceId);
    std::vector<DeviceInfo> getOnlineDevices();
    std::vector<DeviceInfo> getOfflineDevices();
    
private:
    void monitoringLoop();
    void updateDeviceStatus();
    
    std::vector<std::shared_ptr<TPLinkDevice>> devices_;
    std::mutex devices_mutex_;
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_active_;
    std::atomic<bool> should_stop_;
};
