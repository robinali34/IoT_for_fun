# Quick Start Guide - TP-Link Device Controller

## Prerequisites

### For Ubuntu 24 (x64) or Raspberry Pi (ARM)

Install the required dependencies:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libsqlite3-dev libssl-dev libjsoncpp-dev git wget
```

### Architecture Support
- ✅ **x64 (Ubuntu 24)**: Full support with native optimizations
- ✅ **ARM64 (Raspberry Pi 4)**: Full support with ARM-specific optimizations
- ✅ **ARM32 (Raspberry Pi 3)**: Full support with ARM32 optimizations

## Build and Run

1. **Build the project:**
   ```bash
   ./build.sh
   ```

2. **Or build manually:**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j4
   ```

3. **Run the application:**
   ```bash
   ./tplink_controller
   ```

## Quick Test

1. **Discover devices:**
   ```bash
   curl -X POST http://localhost:8080/api/discover
   ```

2. **List devices:**
   ```bash
   curl http://localhost:8080/api/devices
   ```

3. **Control a device (replace DEVICE_ID):**
   ```bash
   # Turn on
   curl -X POST http://localhost:8080/api/devices/DEVICE_ID/power -H "Content-Type: application/json" -d '{"on": true}'
   
   # Set brightness
   curl -X POST http://localhost:8080/api/devices/DEVICE_ID/brightness -H "Content-Type: application/json" -d '{"brightness": 50}'
   ```

## Python Client Example

```bash
# Make sure you have Python 3 and requests library
pip3 install requests

# Run the example client
python3 examples/simple_client.py discover
python3 examples/simple_client.py list
python3 examples/simple_client.py demo
```

## API Endpoints

- `GET /health` - Health check
- `POST /api/discover` - Discover devices
- `GET /api/devices` - List all devices
- `GET /api/devices/{id}` - Get device info
- `POST /api/devices/{id}/power` - Control power
- `POST /api/devices/{id}/brightness` - Set brightness
- `POST /api/devices/{id}/color` - Set color
- `POST /api/devices/{id}/colortemp` - Set color temperature
- `GET /api/stats` - Get statistics

## Troubleshooting

- **Build fails**: Check all dependencies are installed
- **No devices found**: Ensure TP-Link devices are on the same network
- **Connection refused**: Check if port 8080 is available
- **Database errors**: Check file permissions in the project directory

## Next Steps

- Read the full README.md for detailed documentation
- Customize config.json for your network setup
- Install as a system service using the install script
- Integrate with home automation systems
