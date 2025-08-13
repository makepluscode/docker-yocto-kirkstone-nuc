"""Configuration for Hawkbit server."""

import os
from pathlib import Path

# Server configuration
HOST = os.getenv("HAWKBIT_HOST", "0.0.0.0")
PORT = int(os.getenv("HAWKBIT_PORT", "8080"))
DEBUG = os.getenv("HAWKBIT_DEBUG", "false").lower() == "true"

# Bundle configuration
BUNDLE_DIR = os.getenv("HAWKBIT_BUNDLE_DIR", "bundle")
BUNDLE_DIR_PATH = Path(BUNDLE_DIR)

# Logging configuration
LOG_LEVEL = os.getenv("HAWKBIT_LOG_LEVEL", "INFO")
LOG_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"

# CORS configuration
CORS_ORIGINS = os.getenv("HAWKBIT_CORS_ORIGINS", "*").split(",")

# Default tenant
DEFAULT_TENANT = os.getenv("HAWKBIT_DEFAULT_TENANT", "default")

# Server info
SERVER_NAME = "Hawkbit Server"
SERVER_VERSION = "0.1.0"
SERVER_DESCRIPTION = "Simple Hawkbit server implementation for OTA updates" 