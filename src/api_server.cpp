#include "api_server.h"
#include "../third_party/httplib.h"
#include <iostream>
#include <sstream>
#include <json/json.h>

APIServer::APIServer(int port) 
    : port_(port), deviceManager_(nullptr), database_(nullptr), 
      running_(false), should_stop_(false), server_(nullptr) {
}

APIServer::~APIServer() {
    stop();
}

bool APIServer::start() {
    if (running_) {
        return true;
    }
    
    if (!deviceManager_ || !database_) {
        std::cerr << "DeviceManager and Database must be set before starting server" << std::endl;
        return false;
    }
    
    should_stop_ = false;
    server_thread_ = std::thread(&APIServer::runServer, this);
    
    // Wait a moment for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return running_;
}

void APIServer::stop() {
    if (!running_) {
        return;
    }
    
    should_stop_ = true;
    if (server_) {
        static_cast<httplib::Server*>(server_)->stop();
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    if (server_) {
        delete static_cast<httplib::Server*>(server_);
        server_ = nullptr;
    }
    
    running_ = false;
}

bool APIServer::isRunning() {
    return running_;
}

void APIServer::setPort(int port) {
    port_ = port;
}

void APIServer::setDeviceManager(std::shared_ptr<DeviceManager> deviceManager) {
    deviceManager_ = deviceManager;
}

void APIServer::setDatabase(std::shared_ptr<Database> database) {
    database_ = database;
}

void APIServer::setupRoutes() {
    httplib::Server* server = static_cast<httplib::Server*>(server_);
    
    // CORS headers
    server->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });
    
    // Health check
    server->Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
    
    // Device discovery
    server->Post("/api/discover", [this](const httplib::Request&, httplib::Response& res) {
        try {
            auto devices = deviceManager_->discoverDevices();
            
            // Save discovered devices to database
            for (const auto& device : devices) {
                database_->addDevice(device);
                database_->addDiscoveryRecord(device.ip, device.deviceId, device.model, true);
            }
            
            Json::Value response;
            response["success"] = true;
            response["count"] = static_cast<int>(devices.size());
            
            Json::Value devicesArray(Json::arrayValue);
            for (const auto& device : devices) {
                Json::Value deviceJson;
                deviceJson["deviceId"] = device.deviceId;
                deviceJson["name"] = device.name;
                deviceJson["ip"] = device.ip;
                deviceJson["port"] = device.port;
                deviceJson["model"] = device.model;
                deviceJson["mac"] = device.mac;
                deviceJson["isOnline"] = device.isOnline;
                deviceJson["isOn"] = device.isOn;
                deviceJson["brightness"] = device.brightness;
                deviceJson["colorTemp"] = device.colorTemp;
                deviceJson["hue"] = device.hue;
                deviceJson["saturation"] = device.saturation;
                devicesArray.append(deviceJson);
            }
            response["devices"] = devicesArray;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Get all devices
    server->Get("/api/devices", [this](const httplib::Request&, httplib::Response& res) {
        try {
            auto devices = database_->getAllDevices();
            
            Json::Value response;
            response["success"] = true;
            response["count"] = static_cast<int>(devices.size());
            
            Json::Value devicesArray(Json::arrayValue);
            for (const auto& device : devices) {
                Json::Value deviceJson;
                deviceJson["deviceId"] = device.deviceId;
                deviceJson["name"] = device.name;
                deviceJson["ip"] = device.ip;
                deviceJson["port"] = device.port;
                deviceJson["model"] = device.model;
                deviceJson["mac"] = device.mac;
                deviceJson["isOnline"] = device.isOnline;
                deviceJson["isOn"] = device.isOn;
                deviceJson["brightness"] = device.brightness;
                deviceJson["colorTemp"] = device.colorTemp;
                deviceJson["hue"] = device.hue;
                deviceJson["saturation"] = device.saturation;
                devicesArray.append(deviceJson);
            }
            response["devices"] = devicesArray;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Get device by ID
    server->Get("/api/devices/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string deviceId = req.matches[1];
            auto device = database_->getDevice(deviceId);
            
            if (device.deviceId.empty()) {
                res.status = 404;
                res.set_content("{\"success\":false,\"error\":\"Device not found\"}", "application/json");
                return;
            }
            
            Json::Value response;
            response["success"] = true;
            response["device"]["deviceId"] = device.deviceId;
            response["device"]["name"] = device.name;
            response["device"]["ip"] = device.ip;
            response["device"]["port"] = device.port;
            response["device"]["model"] = device.model;
            response["device"]["mac"] = device.mac;
            response["device"]["isOnline"] = device.isOnline;
            response["device"]["isOn"] = device.isOn;
            response["device"]["brightness"] = device.brightness;
            response["device"]["colorTemp"] = device.colorTemp;
            response["device"]["hue"] = device.hue;
            response["device"]["saturation"] = device.saturation;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Control device - turn on/off
    server->Post("/api/devices/(.*)/power", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string deviceId = req.matches[1];
            
            Json::Value request;
            Json::Reader reader;
            if (!reader.parse(req.body, request)) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Invalid JSON\"}", "application/json");
                return;
            }
            
            bool turnOn = request.get("on", false).asBool();
            bool success = turnOn ? deviceManager_->turnOnDevice(deviceId) : deviceManager_->turnOffDevice(deviceId);
            
            if (success) {
                database_->updateDeviceState(deviceId, turnOn);
            }
            
            Json::Value response;
            response["success"] = success;
            response["on"] = turnOn;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Control device brightness
    server->Post("/api/devices/(.*)/brightness", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string deviceId = req.matches[1];
            
            Json::Value request;
            Json::Reader reader;
            if (!reader.parse(req.body, request)) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Invalid JSON\"}", "application/json");
                return;
            }
            
            int brightness = request.get("brightness", 0).asInt();
            if (brightness < 0 || brightness > 100) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Brightness must be between 0 and 100\"}", "application/json");
                return;
            }
            
            bool success = deviceManager_->setDeviceBrightness(deviceId, brightness);
            
            if (success) {
                database_->updateDeviceState(deviceId, brightness > 0, brightness);
            }
            
            Json::Value response;
            response["success"] = success;
            response["brightness"] = brightness;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Control device color
    server->Post("/api/devices/(.*)/color", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string deviceId = req.matches[1];
            
            Json::Value request;
            Json::Reader reader;
            if (!reader.parse(req.body, request)) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Invalid JSON\"}", "application/json");
                return;
            }
            
            int hue = request.get("hue", 0).asInt();
            int saturation = request.get("saturation", 0).asInt();
            int value = request.get("value", 100).asInt();
            
            if (hue < 0 || hue > 360 || saturation < 0 || saturation > 100 || value < 0 || value > 100) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Invalid color values\"}", "application/json");
                return;
            }
            
            bool success = deviceManager_->setDeviceColor(deviceId, hue, saturation, value);
            
            if (success) {
                database_->updateDeviceState(deviceId, value > 0, value, -1, hue, saturation);
            }
            
            Json::Value response;
            response["success"] = success;
            response["hue"] = hue;
            response["saturation"] = saturation;
            response["value"] = value;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Control device color temperature
    server->Post("/api/devices/(.*)/colortemp", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string deviceId = req.matches[1];
            
            Json::Value request;
            Json::Reader reader;
            if (!reader.parse(req.body, request)) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Invalid JSON\"}", "application/json");
                return;
            }
            
            int colorTemp = request.get("colorTemp", 4000).asInt();
            if (colorTemp < 2700 || colorTemp > 6500) {
                res.status = 400;
                res.set_content("{\"success\":false,\"error\":\"Color temperature must be between 2700 and 6500\"}", "application/json");
                return;
            }
            
            bool success = deviceManager_->setDeviceColorTemp(deviceId, colorTemp);
            
            if (success) {
                database_->updateDeviceState(deviceId, true, -1, colorTemp);
            }
            
            Json::Value response;
            response["success"] = success;
            response["colorTemp"] = colorTemp;
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
    
    // Get statistics
    server->Get("/api/stats", [this](const httplib::Request&, httplib::Response& res) {
        try {
            Json::Value response;
            response["success"] = true;
            response["totalDevices"] = database_->getDeviceCount();
            response["onlineDevices"] = database_->getOnlineDeviceCount();
            response["offlineDevices"] = database_->getOfflineDeviceCount();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, response), "application/json");
        } catch (const std::exception& e) {
            Json::Value error;
            error["success"] = false;
            error["error"] = e.what();
            
            Json::StreamWriterBuilder builder;
            res.set_content(Json::writeString(builder, error), "application/json");
            res.status = 500;
        }
    });
}

void APIServer::runServer() {
    server_ = new httplib::Server();
    setupRoutes();
    
    std::cout << "Starting API server on port " << port_ << std::endl;
    running_ = true;
    
    if (!static_cast<httplib::Server*>(server_)->listen("0.0.0.0", port_)) {
        std::cerr << "Failed to start server on port " << port_ << std::endl;
        running_ = false;
    }
}
