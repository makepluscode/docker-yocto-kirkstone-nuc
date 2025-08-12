#!/bin/bash

set -e

echo "Deploying rauc-hawkbit-cpp..."

# Build the application
./build.sh

# Copy binary to target location
sudo cp build/rauc-hawkbit-cpp /usr/local/bin/

# Copy service file
sudo cp services/rauc-hawkbit-cpp.service /etc/systemd/system/

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable rauc-hawkbit-cpp.service

echo "Deployment completed!"
echo "Service enabled and ready to start on boot"
echo "To start immediately: sudo systemctl start rauc-hawkbit-cpp" 