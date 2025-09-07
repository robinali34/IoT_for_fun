#include "tplink_device.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <algorithm>

// Kasa protocol encryption key and IV
const uint8_t TPLinkDevice::kasa_key_[16] = {
    0x09, 0x76, 0x28, 0x34, 0x3f, 0xe9, 0x9e, 0x23,
    0x76, 0x5c, 0x15, 0x13, 0xac, 0xcf, 0x8b, 0x02
};

const uint8_t TPLinkDevice::kasa_iv_[16] = {
    0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x20,
    0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x20
};

TPLinkDevice::TPLinkDevice(const std::string& ip, int port) 
    : ip_(ip), port_(port), socket_fd_(-1), connected_(false) {
    deviceInfo_.ip = ip;
    deviceInfo_.port = port;
    deviceInfo_.isOnline = false;
    deviceInfo_.isOn = false;
    deviceInfo_.brightness = 0;
    deviceInfo_.colorTemp = 4000;
    deviceInfo_.hue = 0;
    deviceInfo_.saturation = 0;
}

TPLinkDevice::~TPLinkDevice() {
    disconnect();
}

bool TPLinkDevice::discover() {
    if (connect()) {
        std::string response = sendCommand("{\"system\":{\"get_sysinfo\":null}}");
        if (!response.empty()) {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(response, root)) {
                if (root.isMember("system") && root["system"].isMember("get_sysinfo")) {
                    Json::Value sysinfo = root["system"]["get_sysinfo"];
                    
                    deviceInfo_.deviceId = sysinfo.get("deviceId", "").asString();
                    deviceInfo_.name = sysinfo.get("alias", "").asString();
                    deviceInfo_.model = sysinfo.get("model", "").asString();
                    deviceInfo_.mac = sysinfo.get("mac", "").asString();
                    deviceInfo_.isOnline = true;
                    
                    // Parse device state
                    if (sysinfo.isMember("light_state")) {
                        Json::Value lightState = sysinfo["light_state"];
                        deviceInfo_.isOn = lightState.get("on_off", 0).asInt() == 1;
                        deviceInfo_.brightness = lightState.get("brightness", 0).asInt();
                        deviceInfo_.colorTemp = lightState.get("color_temp", 4000).asInt();
                        deviceInfo_.hue = lightState.get("hue", 0).asInt();
                        deviceInfo_.saturation = lightState.get("saturation", 0).asInt();
                    } else if (sysinfo.isMember("relay_state")) {
                        deviceInfo_.isOn = sysinfo.get("relay_state", 0).asInt() == 1;
                    }
                    
                    return true;
                }
            }
        }
        disconnect();
    }
    return false;
}

bool TPLinkDevice::connect() {
    if (connected_) {
        return true;
    }
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr);
    
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    return true;
}

void TPLinkDevice::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

bool TPLinkDevice::turnOn() {
    return sendCommand("{\"system\":{\"set_relay_state\":{\"state\":1}}}") != "";
}

bool TPLinkDevice::turnOff() {
    return sendCommand("{\"system\":{\"set_relay_state\":{\"state\":0}}}") != "";
}

bool TPLinkDevice::toggle() {
    return sendCommand("{\"system\":{\"set_relay_state\":{\"state\":-1}}}") != "";
}

bool TPLinkDevice::setBrightness(int brightness) {
    if (brightness < 0 || brightness > 100) {
        return false;
    }
    
    Json::Value params;
    params["brightness"] = brightness;
    params["on_off"] = brightness > 0 ? 1 : 0;
    
    Json::Value command;
    command["smartlife.iot.smartbulb.lightingservice"]["set_light_state"] = params;
    
    Json::StreamWriterBuilder builder;
    std::string commandStr = Json::writeString(builder, command);
    
    return sendCommand(commandStr) != "";
}

bool TPLinkDevice::setColorTemp(int temp) {
    if (temp < 2700 || temp > 6500) {
        return false;
    }
    
    Json::Value params;
    params["color_temp"] = temp;
    params["on_off"] = 1;
    
    Json::Value command;
    command["smartlife.iot.smartbulb.lightingservice"]["set_light_state"] = params;
    
    Json::StreamWriterBuilder builder;
    std::string commandStr = Json::writeString(builder, command);
    
    return sendCommand(commandStr) != "";
}

bool TPLinkDevice::setColor(int hue, int saturation, int value) {
    if (hue < 0 || hue > 360 || saturation < 0 || saturation > 100 || value < 0 || value > 100) {
        return false;
    }
    
    Json::Value params;
    params["hue"] = hue;
    params["saturation"] = saturation;
    params["brightness"] = value;
    params["on_off"] = 1;
    
    Json::Value command;
    command["smartlife.iot.smartbulb.lightingservice"]["set_light_state"] = params;
    
    Json::StreamWriterBuilder builder;
    std::string commandStr = Json::writeString(builder, command);
    
    return sendCommand(commandStr) != "";
}

DeviceInfo TPLinkDevice::getDeviceInfo() {
    return deviceInfo_;
}

bool TPLinkDevice::isOnline() {
    return connected_ && deviceInfo_.isOnline;
}

bool TPLinkDevice::isOn() {
    return deviceInfo_.isOn;
}

int TPLinkDevice::getBrightness() {
    return deviceInfo_.brightness;
}

int TPLinkDevice::getColorTemp() {
    return deviceInfo_.colorTemp;
}

std::string TPLinkDevice::sendCommand(const std::string& command) {
    if (!connect()) {
        return "";
    }
    
    std::string encrypted = encrypt(command);
    if (encrypted.empty()) {
        return "";
    }
    
    // Send command length first (4 bytes, big-endian)
    uint32_t length = htonl(encrypted.length());
    if (send(socket_fd_, &length, 4, 0) != 4) {
        return "";
    }
    
    // Send encrypted command
    if (send(socket_fd_, encrypted.c_str(), encrypted.length(), 0) != (ssize_t)encrypted.length()) {
        return "";
    }
    
    // Receive response length
    uint32_t responseLength;
    if (recv(socket_fd_, &responseLength, 4, 0) != 4) {
        return "";
    }
    responseLength = ntohl(responseLength);
    
    // Receive encrypted response
    std::string encryptedResponse(responseLength, 0);
    if (recv(socket_fd_, &encryptedResponse[0], responseLength, 0) != (ssize_t)responseLength) {
        return "";
    }
    
    return decrypt(encryptedResponse);
}

std::string TPLinkDevice::encrypt(const std::string& data) {
    std::string result;
    result.reserve(data.length() + 16);
    
    // Simple XOR encryption (Kasa protocol uses a more complex scheme, but this is simplified)
    for (size_t i = 0; i < data.length(); ++i) {
        result += data[i] ^ kasa_key_[i % 16];
    }
    
    return result;
}

std::string TPLinkDevice::decrypt(const std::string& data) {
    std::string result;
    result.reserve(data.length());
    
    // Simple XOR decryption (same as encryption for XOR)
    for (size_t i = 0; i < data.length(); ++i) {
        result += data[i] ^ kasa_key_[i % 16];
    }
    
    return result;
}
