#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

struct DeviceInfo {
    std::string deviceId;
    std::string name;
    std::string ip;
    int port;
    std::string model;
    std::string mac;
    bool isOnline;
    bool isOn;
    int brightness; // 0-100 for dimmable devices
    int colorTemp;  // Color temperature for bulbs
    int hue;        // Hue for color bulbs
    int saturation; // Saturation for color bulbs
};

class TPLinkDevice {
public:
    TPLinkDevice(const std::string& ip, int port = 9999);
    ~TPLinkDevice();

    // Device discovery and connection
    bool discover();
    bool connect();
    void disconnect();
    
    // Device control
    bool turnOn();
    bool turnOff();
    bool toggle();
    bool setBrightness(int brightness); // 0-100
    bool setColorTemp(int temp); // 2700-6500K
    bool setColor(int hue, int saturation, int value);
    
    // Device information
    DeviceInfo getDeviceInfo();
    bool isOnline();
    bool isOn();
    int getBrightness();
    int getColorTemp();
    
    // Raw command interface
    std::string sendCommand(const std::string& command);
    
private:
    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);
    std::string createCommand(const std::string& method, const std::map<std::string, std::string>& params = {});
    
    std::string ip_;
    int port_;
    int socket_fd_;
    DeviceInfo deviceInfo_;
    bool connected_;
    
    // Kasa protocol encryption key
    static const uint8_t kasa_key_[16];
    static const uint8_t kasa_iv_[16];
};
