#!/bin/bash

echo "Starting DLT receive for rauc-hawkbit-cpp..."

# Start DLT receive with filter for rauc-hawkbit-cpp
dlt-receive -a RHCP -c HAWK -c RAUC -v 