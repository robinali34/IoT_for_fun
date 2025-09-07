#!/bin/bash

# Installation script for TP-Link Device Controller

set -e

echo "Installing TP-Link Device Controller..."

# Create application directory
sudo mkdir -p /opt/tplink-controller
sudo cp tplink_controller /opt/tplink-controller/

# Create systemd service file
sudo tee /etc/systemd/system/tplink-controller.service > /dev/null << 'SERVICE_EOF'
[Unit]
Description=TP-Link Device Controller
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/opt/tplink-controller
ExecStart=/opt/tplink-controller/tplink_controller
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable tplink-controller

echo "Installation complete!"
echo "To start the service: sudo systemctl start tplink-controller"
echo "To check status: sudo systemctl status tplink-controller"
echo "To view logs: journalctl -u tplink-controller -f"
