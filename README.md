# TP-Link Device Controller

A cross-platform C++ application for controlling TP-Link smart devices (light bulbs, switches, etc.) on Ubuntu 24 (x64) and Raspberry Pi (ARM) using RESTful API and SQLite database.

## Features

- **Device Discovery**: Automatically discover TP-Link devices on your network
- **Device Control**: Turn on/off, adjust brightness, set colors, and control color temperature
- **RESTful API**: HTTP API for remote control and integration
- **SQLite Database**: Persistent storage of device information and states
- **Real-time Monitoring**: Continuous monitoring of device status
- **Cross-Platform**: Optimized for both x64 (Ubuntu 24) and ARM (Raspberry Pi)

## Supported Devices

- TP-Link Kasa smart light bulbs (dimmable, color, white spectrum)
- TP-Link Kasa smart switches and outlets
- Other TP-Link Kasa compatible devices

## Prerequisites

### On Ubuntu 24 (x64) or Raspberry Pi (ARM)

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y build-essential cmake pkg-config
sudo apt install -y libsqlite3-dev libssl-dev libjsoncpp-dev
sudo apt install -y git wget
```

### Architecture Detection
The build system automatically detects your architecture and applies appropriate optimizations:
- **x64 (Ubuntu 24)**: Uses native x64 optimizations
- **ARM64 (Raspberry Pi 4)**: Uses ARM64 NEON optimizations
- **ARM32 (Raspberry Pi 3)**: Uses ARM32 NEON optimizations

## Building

### Cross-Platform Build (Recommended)

1. **Clone or download the project**:
   ```bash
   git clone <repository-url>
   cd IoT_for_fun
   ```

2. **Build for your platform**:
   ```bash
   # Automatically detects architecture and applies optimizations
   ./build_cross_platform.sh
   ```

3. **Run the application**:
   ```bash
   cd build
   ./tplink_controller
   ```

### Manual Build

1. **Create build directory**:
   ```bash
   mkdir build && cd build
   ```

2. **Configure and build**:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

3. **Install as service (optional)**:
   ```bash
   ./install.sh
   ```

## Usage

### Basic Usage

```bash
# Start the server with default settings
./tplink_controller

# Start with custom port
./tplink_controller --port 8080

# Start with custom database path
./tplink_controller --database /path/to/devices.db

# Enable verbose logging
./tplink_controller --verbose

# Only discover devices and exit
./tplink_controller --discover-only

# Disable device monitoring
./tplink_controller --no-monitoring
```

### Command Line Options

- `-p, --port PORT`: API server port (default: 8080)
- `-d, --database PATH`: Database file path (default: tplink_devices.db)
- `-h, --help`: Show help message
- `-v, --verbose`: Enable verbose logging
- `--discover-only`: Only discover devices and exit
- `--no-monitoring`: Disable device monitoring

## API Endpoints

### Health Check
```
GET /health
```
Returns server status.

### Device Discovery
```
POST /api/discover
```
Discovers TP-Link devices on the network.

### Get All Devices
```
GET /api/devices
```
Returns list of all known devices.

### Get Device by ID
```
GET /api/devices/{deviceId}
```
Returns specific device information.

### Control Device Power
```
POST /api/devices/{deviceId}/power
Content-Type: application/json

{
  "on": true
}
```

### Control Device Brightness
```
POST /api/devices/{deviceId}/brightness
Content-Type: application/json

{
  "brightness": 75
}
```

### Control Device Color
```
POST /api/devices/{deviceId}/color
Content-Type: application/json

{
  "hue": 120,
  "saturation": 100,
  "value": 80
}
```

### Control Device Color Temperature
```
POST /api/devices/{deviceId}/colortemp
Content-Type: application/json

{
  "colorTemp": 4000
}
```

### Get Statistics
```
GET /api/stats
```
Returns device statistics.

## Example Usage

### Using curl

```bash
# Discover devices
curl -X POST http://localhost:8080/api/discover

# Get all devices
curl http://localhost:8080/api/devices

# Turn on a device
curl -X POST http://localhost:8080/api/devices/DEVICE_ID/power \
  -H "Content-Type: application/json" \
  -d '{"on": true}'

# Set brightness to 50%
curl -X POST http://localhost:8080/api/devices/DEVICE_ID/brightness \
  -H "Content-Type: application/json" \
  -d '{"brightness": 50}'

# Set color to green
curl -X POST http://localhost:8080/api/devices/DEVICE_ID/color \
  -H "Content-Type: application/json" \
  -d '{"hue": 120, "saturation": 100, "value": 80}'
```

### Using Python

```python
import requests
import json

# Discover devices
response = requests.post('http://localhost:8080/api/discover')
devices = response.json()

# Get all devices
response = requests.get('http://localhost:8080/api/devices')
devices = response.json()

# Control a device
device_id = "your_device_id"
requests.post(f'http://localhost:8080/api/devices/{device_id}/power', 
              json={"on": True})
```

## Database Schema

The application uses SQLite with the following tables:

### devices
- `device_id`: Unique device identifier
- `name`: Device name/alias
- `ip`: Device IP address
- `port`: Device port (usually 9999)
- `model`: Device model
- `mac`: Device MAC address
- `is_online`: Online status
- `is_on`: Power state
- `brightness`: Brightness level (0-100)
- `color_temp`: Color temperature (2700-6500K)
- `hue`: Hue value (0-360)
- `saturation`: Saturation value (0-100)
- `created_at`: Creation timestamp
- `updated_at`: Last update timestamp

### discovery_history
- `id`: Auto-increment ID
- `ip`: Discovered IP address
- `device_id`: Associated device ID
- `model`: Device model
- `success`: Discovery success status
- `discovered_at`: Discovery timestamp

## Troubleshooting

### Device Discovery Issues
1. Ensure devices are on the same network as Raspberry Pi
2. Check firewall settings
3. Try manual device addition with known IP addresses

### Build Issues
1. Ensure all dependencies are installed
2. Check CMake version (3.10+ required)
3. Verify C++17 compiler support

### Runtime Issues
1. Check database file permissions
2. Ensure port 8080 is available
3. Check device connectivity

## Security Notes

- The API server binds to all interfaces (0.0.0.0) by default
- Consider implementing authentication for production use
- Use HTTPS in production environments
- Regularly update dependencies for security patches

## License

This project is provided as-is for educational and personal use.

## Contributing

Feel free to submit issues and enhancement requests!
