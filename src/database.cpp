#include "database.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

Database::Database(const std::string& dbPath) : dbPath_(dbPath), db_(nullptr) {
}

Database::~Database() {
    if (db_) {
        sqlite3_close(static_cast<sqlite3*>(db_));
    }
}

bool Database::initialize() {
    sqlite3* db;
    if (sqlite3_open(dbPath_.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    db_ = db;
    
    // Create devices table
    std::string createDevicesTable = R"(
        CREATE TABLE IF NOT EXISTS devices (
            device_id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            ip TEXT NOT NULL,
            port INTEGER NOT NULL,
            model TEXT,
            mac TEXT,
            is_online INTEGER DEFAULT 0,
            is_on INTEGER DEFAULT 0,
            brightness INTEGER DEFAULT 0,
            color_temp INTEGER DEFAULT 4000,
            hue INTEGER DEFAULT 0,
            saturation INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!executeQuery(createDevicesTable)) {
        return false;
    }
    
    // Create discovery_history table
    std::string createDiscoveryTable = R"(
        CREATE TABLE IF NOT EXISTS discovery_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ip TEXT NOT NULL,
            device_id TEXT,
            model TEXT,
            success INTEGER DEFAULT 0,
            discovered_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!executeQuery(createDiscoveryTable)) {
        return false;
    }
    
    // Create index for faster lookups
    std::string createIndex = "CREATE INDEX IF NOT EXISTS idx_devices_ip ON devices(ip)";
    executeQuery(createIndex);
    
    return true;
}

bool Database::isOpen() {
    return db_ != nullptr;
}

bool Database::addDevice(const DeviceInfo& device) {
    std::stringstream query;
    query << "INSERT OR REPLACE INTO devices (device_id, name, ip, port, model, mac, "
          << "is_online, is_on, brightness, color_temp, hue, saturation, updated_at) "
          << "VALUES ('" << device.deviceId << "', '" << device.name << "', '"
          << device.ip << "', " << device.port << ", '" << device.model << "', '"
          << device.mac << "', " << (device.isOnline ? 1 : 0) << ", "
          << (device.isOn ? 1 : 0) << ", " << device.brightness << ", "
          << device.colorTemp << ", " << device.hue << ", " << device.saturation
          << ", CURRENT_TIMESTAMP)";
    
    return executeQuery(query.str());
}

bool Database::updateDevice(const DeviceInfo& device) {
    return addDevice(device); // INSERT OR REPLACE handles updates
}

bool Database::removeDevice(const std::string& deviceId) {
    std::string query = "DELETE FROM devices WHERE device_id = '" + deviceId + "'";
    return executeQuery(query);
}

DeviceInfo Database::getDevice(const std::string& deviceId) {
    DeviceInfo device;
    
    std::string query = "SELECT * FROM devices WHERE device_id = '" + deviceId + "'";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        DeviceInfo* device = static_cast<DeviceInfo*>(data);
        
        if (argc >= 13) {
            device->deviceId = argv[0] ? argv[0] : "";
            device->name = argv[1] ? argv[1] : "";
            device->ip = argv[2] ? argv[2] : "";
            device->port = argv[3] ? std::stoi(argv[3]) : 9999;
            device->model = argv[4] ? argv[4] : "";
            device->mac = argv[5] ? argv[5] : "";
            device->isOnline = argv[6] ? std::stoi(argv[6]) != 0 : false;
            device->isOn = argv[7] ? std::stoi(argv[7]) != 0 : false;
            device->brightness = argv[8] ? std::stoi(argv[8]) : 0;
            device->colorTemp = argv[9] ? std::stoi(argv[9]) : 4000;
            device->hue = argv[10] ? std::stoi(argv[10]) : 0;
            device->saturation = argv[11] ? std::stoi(argv[11]) : 0;
        }
        
        return 0;
    };
    
    executeQueryWithResult(query, callback, &device);
    return device;
}

std::vector<DeviceInfo> Database::getAllDevices() {
    std::vector<DeviceInfo> devices;
    
    std::string query = "SELECT * FROM devices ORDER BY name";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        std::vector<DeviceInfo>* devices = static_cast<std::vector<DeviceInfo>*>(data);
        
        if (argc >= 13) {
            DeviceInfo device;
            device.deviceId = argv[0] ? argv[0] : "";
            device.name = argv[1] ? argv[1] : "";
            device.ip = argv[2] ? argv[2] : "";
            device.port = argv[3] ? std::stoi(argv[3]) : 9999;
            device.model = argv[4] ? argv[4] : "";
            device.mac = argv[5] ? argv[5] : "";
            device.isOnline = argv[6] ? std::stoi(argv[6]) != 0 : false;
            device.isOn = argv[7] ? std::stoi(argv[7]) != 0 : false;
            device.brightness = argv[8] ? std::stoi(argv[8]) : 0;
            device.colorTemp = argv[9] ? std::stoi(argv[9]) : 4000;
            device.hue = argv[10] ? std::stoi(argv[10]) : 0;
            device.saturation = argv[11] ? std::stoi(argv[11]) : 0;
            
            devices->push_back(device);
        }
        
        return 0;
    };
    
    executeQueryWithResult(query, callback, &devices);
    return devices;
}

std::vector<DeviceInfo> Database::getDevicesByStatus(bool isOnline) {
    std::vector<DeviceInfo> devices;
    
    std::string query = "SELECT * FROM devices WHERE is_online = " + 
                       std::string(isOnline ? "1" : "0") + " ORDER BY name";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        std::vector<DeviceInfo>* devices = static_cast<std::vector<DeviceInfo>*>(data);
        
        if (argc >= 13) {
            DeviceInfo device;
            device.deviceId = argv[0] ? argv[0] : "";
            device.name = argv[1] ? argv[1] : "";
            device.ip = argv[2] ? argv[2] : "";
            device.port = argv[3] ? std::stoi(argv[3]) : 9999;
            device.model = argv[4] ? argv[4] : "";
            device.mac = argv[5] ? argv[5] : "";
            device.isOnline = argv[6] ? std::stoi(argv[6]) != 0 : false;
            device.isOn = argv[7] ? std::stoi(argv[7]) != 0 : false;
            device.brightness = argv[8] ? std::stoi(argv[8]) : 0;
            device.colorTemp = argv[9] ? std::stoi(argv[9]) : 4000;
            device.hue = argv[10] ? std::stoi(argv[10]) : 0;
            device.saturation = argv[11] ? std::stoi(argv[11]) : 0;
            
            devices->push_back(device);
        }
        
        return 0;
    };
    
    executeQueryWithResult(query, callback, &devices);
    return devices;
}

bool Database::updateDeviceStatus(const std::string& deviceId, bool isOnline) {
    std::string query = "UPDATE devices SET is_online = " + 
                       std::string(isOnline ? "1" : "0") + 
                       ", updated_at = CURRENT_TIMESTAMP WHERE device_id = '" + deviceId + "'";
    return executeQuery(query);
}

bool Database::updateDeviceState(const std::string& deviceId, bool isOn, int brightness, 
                                int colorTemp, int hue, int saturation) {
    std::stringstream query;
    query << "UPDATE devices SET is_on = " << (isOn ? 1 : 0);
    
    if (brightness >= 0) {
        query << ", brightness = " << brightness;
    }
    if (colorTemp >= 0) {
        query << ", color_temp = " << colorTemp;
    }
    if (hue >= 0) {
        query << ", hue = " << hue;
    }
    if (saturation >= 0) {
        query << ", saturation = " << saturation;
    }
    
    query << ", updated_at = CURRENT_TIMESTAMP WHERE device_id = '" << deviceId << "'";
    
    return executeQuery(query.str());
}

bool Database::addDiscoveryRecord(const std::string& ip, const std::string& deviceId, 
                                 const std::string& model, bool success) {
    std::stringstream query;
    query << "INSERT INTO discovery_history (ip, device_id, model, success) "
          << "VALUES ('" << ip << "', '" << deviceId << "', '" << model << "', "
          << (success ? 1 : 0) << ")";
    
    return executeQuery(query.str());
}

std::vector<std::string> Database::getKnownIPs() {
    std::vector<std::string> ips;
    
    std::string query = "SELECT DISTINCT ip FROM devices";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        std::vector<std::string>* ips = static_cast<std::vector<std::string>*>(data);
        if (argc > 0 && argv[0]) {
            ips->push_back(argv[0]);
        }
        return 0;
    };
    
    executeQueryWithResult(query, callback, &ips);
    return ips;
}

int Database::getDeviceCount() {
    int count = 0;
    
    std::string query = "SELECT COUNT(*) FROM devices";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        int* count = static_cast<int*>(data);
        if (argc > 0 && argv[0]) {
            *count = std::stoi(argv[0]);
        }
        return 0;
    };
    
    executeQueryWithResult(query, callback, &count);
    return count;
}

int Database::getOnlineDeviceCount() {
    int count = 0;
    
    std::string query = "SELECT COUNT(*) FROM devices WHERE is_online = 1";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        int* count = static_cast<int*>(data);
        if (argc > 0 && argv[0]) {
            *count = std::stoi(argv[0]);
        }
        return 0;
    };
    
    executeQueryWithResult(query, callback, &count);
    return count;
}

int Database::getOfflineDeviceCount() {
    int count = 0;
    
    std::string query = "SELECT COUNT(*) FROM devices WHERE is_online = 0";
    
    auto callback = [](void* data, int argc, char** argv, char** colNames) -> int {
        int* count = static_cast<int*>(data);
        if (argc > 0 && argv[0]) {
            *count = std::stoi(argv[0]);
        }
        return 0;
    };
    
    executeQueryWithResult(query, callback, &count);
    return count;
}

bool Database::executeQuery(const std::string& query) {
    if (!db_) {
        return false;
    }
    
    char* errMsg = 0;
    int rc = sqlite3_exec(static_cast<sqlite3*>(db_), query.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Database::executeQueryWithResult(const std::string& query, 
                                    std::function<int(void*, int, char**, char**)> callback,
                                    void* data) {
    if (!db_) {
        return false;
    }
    
    char* errMsg = 0;
    int rc = sqlite3_exec(static_cast<sqlite3*>(db_), query.c_str(), 
                         [](void* data, int argc, char** argv, char** colNames) -> int {
                             auto* callback = static_cast<std::function<int(void*, int, char**, char**)>*>(data);
                             return (*callback)(data, argc, argv, colNames);
                         }, &callback, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}
