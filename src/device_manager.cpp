#include "device_manager.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

DeviceManager::DeviceManager() 
    : monitoring_active_(false), should_stop_(false) {
}

DeviceManager::~DeviceManager() {
    stopMonitoring();
}

std::vector<DeviceInfo> DeviceManager::discoverDevices() {
    std::vector<DeviceInfo> discoveredDevices;
    
    // For now, we'll implement a simple discovery by trying common IP ranges
    // In a real implementation, you'd use UDP broadcast or mDNS
    std::vector<std::string> commonIPs = {
        "192.168.1.100", "192.168.1.101", "192.168.1.102", "192.168.1.103",
        "192.168.0.100", "192.168.0.101", "192.168.0.102", "192.168.0.103",
        "10.0.0.100", "10.0.0.101", "10.0.0.102", "10.0.0.103"
    };
    
    for (const auto& ip : commonIPs) {
        auto device = std::make_shared<TPLinkDevice>(ip);
        if (device->discover()) {
            DeviceInfo info = device->getDeviceInfo();
            discoveredDevices.push_back(info);
            
            // Add to managed devices
            std::lock_guard<std::mutex> lock(devices_mutex_);
            devices_.push_back(device);
        }
    }
    
    return discoveredDevices;
}

bool DeviceManager::addDevice(const std::string& ip, int port) {
    auto device = std::make_shared<TPLinkDevice>(ip, port);
    if (device->discover()) {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        devices_.push_back(device);
        return true;
    }
    return false;
}

bool DeviceManager::removeDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&deviceId](const std::shared_ptr<TPLinkDevice>& device) {
            return device->getDeviceInfo().deviceId == deviceId;
        });
    
    if (it != devices_.end()) {
        devices_.erase(it);
        return true;
    }
    return false;
}

std::vector<DeviceInfo> DeviceManager::getAllDevices() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    std::vector<DeviceInfo> deviceInfos;
    
    for (const auto& device : devices_) {
        deviceInfos.push_back(device->getDeviceInfo());
    }
    
    return deviceInfos;
}

std::shared_ptr<TPLinkDevice> DeviceManager::getDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&deviceId](const std::shared_ptr<TPLinkDevice>& device) {
            return device->getDeviceInfo().deviceId == deviceId;
        });
    
    if (it != devices_.end()) {
        return *it;
    }
    return nullptr;
}

bool DeviceManager::turnOnDevice(const std::string& deviceId) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->turnOn();
    }
    return false;
}

bool DeviceManager::turnOffDevice(const std::string& deviceId) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->turnOff();
    }
    return false;
}

bool DeviceManager::toggleDevice(const std::string& deviceId) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->toggle();
    }
    return false;
}

bool DeviceManager::setDeviceBrightness(const std::string& deviceId, int brightness) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->setBrightness(brightness);
    }
    return false;
}

bool DeviceManager::setDeviceColor(const std::string& deviceId, int hue, int saturation, int value) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->setColor(hue, saturation, value);
    }
    return false;
}

bool DeviceManager::setDeviceColorTemp(const std::string& deviceId, int temp) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->setColorTemp(temp);
    }
    return false;
}

void DeviceManager::startMonitoring() {
    if (monitoring_active_) {
        return;
    }
    
    should_stop_ = false;
    monitoring_active_ = true;
    monitoring_thread_ = std::thread(&DeviceManager::monitoringLoop, this);
}

void DeviceManager::stopMonitoring() {
    if (!monitoring_active_) {
        return;
    }
    
    should_stop_ = true;
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    monitoring_active_ = false;
}

bool DeviceManager::isMonitoring() {
    return monitoring_active_;
}

DeviceInfo DeviceManager::getDeviceInfo(const std::string& deviceId) {
    auto device = getDevice(deviceId);
    if (device) {
        return device->getDeviceInfo();
    }
    return DeviceInfo{};
}

std::vector<DeviceInfo> DeviceManager::getOnlineDevices() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    std::vector<DeviceInfo> onlineDevices;
    
    for (const auto& device : devices_) {
        if (device->isOnline()) {
            onlineDevices.push_back(device->getDeviceInfo());
        }
    }
    
    return onlineDevices;
}

std::vector<DeviceInfo> DeviceManager::getOfflineDevices() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    std::vector<DeviceInfo> offlineDevices;
    
    for (const auto& device : devices_) {
        if (!device->isOnline()) {
            offlineDevices.push_back(device->getDeviceInfo());
        }
    }
    
    return offlineDevices;
}

void DeviceManager::monitoringLoop() {
    while (!should_stop_) {
        updateDeviceStatus();
        std::this_thread::sleep_for(std::chrono::seconds(30)); // Check every 30 seconds
    }
}

void DeviceManager::updateDeviceStatus() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    for (auto& device : devices_) {
        // Try to reconnect and update status
        if (!device->isOnline()) {
            device->discover();
        }
    }
}
