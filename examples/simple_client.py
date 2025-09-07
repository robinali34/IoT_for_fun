#!/usr/bin/env python3
"""
Simple Python client for TP-Link Device Controller
This example demonstrates how to use the RESTful API
"""

import requests
import json
import time
import sys

class TPLinkClient:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url
        self.session = requests.Session()
    
    def discover_devices(self):
        """Discover TP-Link devices on the network"""
        try:
            response = self.session.post(f"{self.base_url}/api/discover")
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error discovering devices: {e}")
            return None
    
    def get_devices(self):
        """Get all known devices"""
        try:
            response = self.session.get(f"{self.base_url}/api/devices")
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error getting devices: {e}")
            return None
    
    def get_device(self, device_id):
        """Get specific device information"""
        try:
            response = self.session.get(f"{self.base_url}/api/devices/{device_id}")
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error getting device {device_id}: {e}")
            return None
    
    def turn_on_device(self, device_id):
        """Turn on a device"""
        try:
            response = self.session.post(
                f"{self.base_url}/api/devices/{device_id}/power",
                json={"on": True}
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error turning on device {device_id}: {e}")
            return None
    
    def turn_off_device(self, device_id):
        """Turn off a device"""
        try:
            response = self.session.post(
                f"{self.base_url}/api/devices/{device_id}/power",
                json={"on": False}
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error turning off device {device_id}: {e}")
            return None
    
    def set_brightness(self, device_id, brightness):
        """Set device brightness (0-100)"""
        try:
            response = self.session.post(
                f"{self.base_url}/api/devices/{device_id}/brightness",
                json={"brightness": brightness}
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error setting brightness for device {device_id}: {e}")
            return None
    
    def set_color(self, device_id, hue, saturation, value):
        """Set device color"""
        try:
            response = self.session.post(
                f"{self.base_url}/api/devices/{device_id}/color",
                json={
                    "hue": hue,
                    "saturation": saturation,
                    "value": value
                }
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error setting color for device {device_id}: {e}")
            return None
    
    def set_color_temp(self, device_id, color_temp):
        """Set device color temperature (2700-6500K)"""
        try:
            response = self.session.post(
                f"{self.base_url}/api/devices/{device_id}/colortemp",
                json={"colorTemp": color_temp}
            )
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error setting color temperature for device {device_id}: {e}")
            return None
    
    def get_stats(self):
        """Get device statistics"""
        try:
            response = self.session.get(f"{self.base_url}/api/stats")
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"Error getting stats: {e}")
            return None

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 simple_client.py <command> [args...]")
        print("Commands:")
        print("  discover                    - Discover devices")
        print("  list                        - List all devices")
        print("  info <device_id>            - Get device info")
        print("  on <device_id>              - Turn on device")
        print("  off <device_id>             - Turn off device")
        print("  brightness <device_id> <0-100> - Set brightness")
        print("  color <device_id> <hue> <sat> <val> - Set color")
        print("  colortemp <device_id> <temp> - Set color temperature")
        print("  stats                       - Get statistics")
        print("  demo                        - Run demo sequence")
        return
    
    client = TPLinkClient()
    command = sys.argv[1].lower()
    
    if command == "discover":
        print("Discovering devices...")
        result = client.discover_devices()
        if result and result.get("success"):
            print(f"Discovered {result.get('count', 0)} devices")
            for device in result.get("devices", []):
                print(f"  - {device['name']} ({device['ip']}) - {device['model']}")
        else:
            print("Discovery failed or no devices found")
    
    elif command == "list":
        result = client.get_devices()
        if result and result.get("success"):
            devices = result.get("devices", [])
            print(f"Found {len(devices)} devices:")
            for device in devices:
                status = "Online" if device.get("isOnline") else "Offline"
                power = "On" if device.get("isOn") else "Off"
                print(f"  - {device['name']} ({device['ip']}) - {status} - {power}")
        else:
            print("Failed to get devices")
    
    elif command == "info" and len(sys.argv) > 2:
        device_id = sys.argv[2]
        result = client.get_device(device_id)
        if result and result.get("success"):
            device = result.get("device", {})
            print(f"Device: {device.get('name', 'Unknown')}")
            print(f"  IP: {device.get('ip', 'Unknown')}")
            print(f"  Model: {device.get('model', 'Unknown')}")
            print(f"  Online: {device.get('isOnline', False)}")
            print(f"  Power: {'On' if device.get('isOn') else 'Off'}")
            print(f"  Brightness: {device.get('brightness', 0)}%")
            print(f"  Color Temp: {device.get('colorTemp', 0)}K")
        else:
            print(f"Failed to get device info for {device_id}")
    
    elif command == "on" and len(sys.argv) > 2:
        device_id = sys.argv[2]
        result = client.turn_on_device(device_id)
        if result and result.get("success"):
            print(f"Device {device_id} turned on")
        else:
            print(f"Failed to turn on device {device_id}")
    
    elif command == "off" and len(sys.argv) > 2:
        device_id = sys.argv[2]
        result = client.turn_off_device(device_id)
        if result and result.get("success"):
            print(f"Device {device_id} turned off")
        else:
            print(f"Failed to turn off device {device_id}")
    
    elif command == "brightness" and len(sys.argv) > 3:
        device_id = sys.argv[2]
        brightness = int(sys.argv[3])
        result = client.set_brightness(device_id, brightness)
        if result and result.get("success"):
            print(f"Device {device_id} brightness set to {brightness}%")
        else:
            print(f"Failed to set brightness for device {device_id}")
    
    elif command == "color" and len(sys.argv) > 5:
        device_id = sys.argv[2]
        hue = int(sys.argv[3])
        saturation = int(sys.argv[4])
        value = int(sys.argv[5])
        result = client.set_color(device_id, hue, saturation, value)
        if result and result.get("success"):
            print(f"Device {device_id} color set to H:{hue} S:{saturation} V:{value}")
        else:
            print(f"Failed to set color for device {device_id}")
    
    elif command == "colortemp" and len(sys.argv) > 3:
        device_id = sys.argv[2]
        color_temp = int(sys.argv[3])
        result = client.set_color_temp(device_id, color_temp)
        if result and result.get("success"):
            print(f"Device {device_id} color temperature set to {color_temp}K")
        else:
            print(f"Failed to set color temperature for device {device_id}")
    
    elif command == "stats":
        result = client.get_stats()
        if result and result.get("success"):
            print("Device Statistics:")
            print(f"  Total devices: {result.get('totalDevices', 0)}")
            print(f"  Online devices: {result.get('onlineDevices', 0)}")
            print(f"  Offline devices: {result.get('offlineDevices', 0)}")
        else:
            print("Failed to get statistics")
    
    elif command == "demo":
        print("Running demo sequence...")
        
        # Discover devices
        print("1. Discovering devices...")
        result = client.discover_devices()
        if not result or not result.get("success"):
            print("Demo failed: No devices discovered")
            return
        
        devices = result.get("devices", [])
        if not devices:
            print("Demo failed: No devices found")
            return
        
        device = devices[0]
        device_id = device["deviceId"]
        print(f"Using device: {device['name']} ({device_id})")
        
        # Turn on device
        print("2. Turning on device...")
        client.turn_on_device(device_id)
        time.sleep(1)
        
        # Set brightness
        print("3. Setting brightness to 50%...")
        client.set_brightness(device_id, 50)
        time.sleep(1)
        
        # Set color (red)
        print("4. Setting color to red...")
        client.set_color(device_id, 0, 100, 80)
        time.sleep(2)
        
        # Set color (green)
        print("5. Setting color to green...")
        client.set_color(device_id, 120, 100, 80)
        time.sleep(2)
        
        # Set color (blue)
        print("6. Setting color to blue...")
        client.set_color(device_id, 240, 100, 80)
        time.sleep(2)
        
        # Set color temperature
        print("7. Setting color temperature to warm white...")
        client.set_color_temp(device_id, 2700)
        time.sleep(2)
        
        # Set brightness to 100%
        print("8. Setting brightness to 100%...")
        client.set_brightness(device_id, 100)
        time.sleep(1)
        
        # Turn off device
        print("9. Turning off device...")
        client.turn_off_device(device_id)
        
        print("Demo completed!")
    
    else:
        print(f"Unknown command: {command}")
        print("Use 'python3 simple_client.py' to see available commands")

if __name__ == "__main__":
    main()
