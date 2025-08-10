"""Configuration constants and settings for RAUC Updater."""

from pathlib import Path
from typing import Dict, Any

# Default connection settings
DEFAULT_HOST = "192.168.1.100"
DEFAULT_USER = "root"
DEFAULT_PORT = 22

# Network settings
DEFAULT_INTERFACE = "enp42s0"
DEFAULT_HOST_IP = "192.168.1.101"
DEFAULT_NETMASK = "255.255.255.0"

# File transfer settings
DEFAULT_REMOTE_PATH = "/tmp"
DEFAULT_TIMEOUT = 600

# Application info
APP_NAME = "ARCRO"
APP_VERSION = "1.0.0"
APP_ORGANIZATION = "ARCRO Tools"
APP_DESCRIPTION = "Advanced RAUC Control & Rollout Operations"

# D-Bus settings
DBUS_DEST = "de.pengutronix.rauc"
DBUS_PATH = "/de/pengutronix/rauc/Installer"
DBUS_INTERFACE = "de.pengutronix.rauc.Installer"

# SSH connection keywords for error detection
SSH_ERROR_KEYWORDS = ["connection", "ssh", "broken pipe", "timeout"]

# Hawkbit default settings
HAWKBIT_DEFAULTS = {
    "tenant": "DEFAULT",
    "ssl": True,
    "bundle_location": "/tmp/hawkbit_bundle.raucb",
    "device_product": "Intel-NUC",
    "device_model": "NUC-Series",
    "device_version": "1.0"
}

# Logging settings
LOG_FORMAT = "%(asctime)s [%(levelname)s] %(message)s"
LOG_DATE_FORMAT = "%Y-%m-%d %H:%M:%S"

def get_default_config() -> Dict[str, Any]:
    """Get default configuration dictionary."""
    return {
        "host": DEFAULT_HOST,
        "user": DEFAULT_USER,
        "port": DEFAULT_PORT,
        "interface": DEFAULT_INTERFACE,
        "host_ip": DEFAULT_HOST_IP,
        "netmask": DEFAULT_NETMASK,
        "remote_path": DEFAULT_REMOTE_PATH,
        "timeout": DEFAULT_TIMEOUT
    }