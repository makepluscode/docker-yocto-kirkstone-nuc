"""Configuration for updater server."""

import os
from pathlib import Path

# Server configuration
HOST = os.getenv("UPDATER_HOST", "0.0.0.0")
PORT = int(os.getenv("UPDATER_PORT", "8080"))
HTTPS_PORT = int(os.getenv("UPDATER_HTTPS_PORT", "8443"))
DEBUG = os.getenv("UPDATER_DEBUG", "false").lower() == "true"
ENABLE_HTTPS = os.getenv("UPDATER_ENABLE_HTTPS", "false").lower() == "true"

# Bundle configuration
BUNDLE_DIR = os.getenv("UPDATER_BUNDLE_DIR", "bundle")
BUNDLE_DIR_PATH = Path(BUNDLE_DIR)

# Logging configuration
LOG_LEVEL = os.getenv("UPDATER_LOG_LEVEL", "INFO")
LOG_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"

# CORS configuration
CORS_ORIGINS = os.getenv("UPDATER_CORS_ORIGINS", "*").split(",")

# Default tenant
DEFAULT_TENANT = os.getenv("UPDATER_DEFAULT_TENANT", "default")

# SSL/TLS configuration
SSL_CERT_FILE = os.getenv("UPDATER_SSL_CERT", "certs/server.crt")
SSL_KEY_FILE = os.getenv("UPDATER_SSL_KEY", "certs/server.key")
SSL_CA_FILE = os.getenv("UPDATER_SSL_CA", "certs/ca.crt")

# GUI configuration
ENABLE_GUI = os.getenv("UPDATER_ENABLE_GUI", "true").lower() == "true"
GUI_PATH = os.getenv("UPDATER_GUI_PATH", "gui")
STATIC_FILES_PATH = os.getenv("UPDATER_STATIC_PATH", "static")

# Server info
SERVER_NAME = "Updater Server"
SERVER_VERSION = "0.2.0"
SERVER_DESCRIPTION = "Simple updater server implementation for OTA updates with HTTPS support"

# Certificate paths
SSL_CERT_PATH = Path(SSL_CERT_FILE)
SSL_KEY_PATH = Path(SSL_KEY_FILE)
SSL_CA_PATH = Path(SSL_CA_FILE)

# GUI paths
GUI_PATH_OBJ = Path(GUI_PATH)
STATIC_PATH_OBJ = Path(STATIC_FILES_PATH)