"""Configuration for updater server."""

import os
from pathlib import Path
from typing import List, Optional
from dataclasses import dataclass, field
from .exceptions import ConfigurationError


@dataclass
class ServerConfig:
    """Server configuration settings."""
    host: str = "0.0.0.0"
    port: int = 8080
    https_port: int = 8443
    debug: bool = False
    enable_https: bool = False
    external_host: Optional[str] = None
    external_url: Optional[str] = None
    
    @classmethod
    def from_env(cls) -> 'ServerConfig':
        """Create config from environment variables."""
        return cls(
            host=os.getenv("UPDATER_HOST", "0.0.0.0"),
            port=int(os.getenv("UPDATER_PORT", "8080")),
            https_port=int(os.getenv("UPDATER_HTTPS_PORT", "8443")),
            debug=os.getenv("UPDATER_DEBUG", "false").lower() == "true",
            enable_https=os.getenv("UPDATER_ENABLE_HTTPS", "false").lower() == "true",
            external_host=os.getenv("UPDATER_EXTERNAL_HOST"),
            external_url=os.getenv("UPDATER_EXTERNAL_URL")
        )


@dataclass
class BundleConfig:
    """Bundle configuration settings."""
    bundle_dir: str = "bundle"
    max_bundle_size: int = 1024 * 1024 * 1024  # 1GB
    allowed_extensions: List[str] = field(default_factory=lambda: [".raucb"])
    
    @property
    def bundle_dir_path(self) -> Path:
        """Get bundle directory as Path object."""
        return Path(self.bundle_dir)
    
    @classmethod
    def from_env(cls) -> 'BundleConfig':
        """Create config from environment variables."""
        max_size_str = os.getenv("UPDATER_MAX_BUNDLE_SIZE", "1073741824")  # 1GB default
        extensions_str = os.getenv("UPDATER_ALLOWED_EXTENSIONS", ".raucb")
        
        return cls(
            bundle_dir=os.getenv("UPDATER_BUNDLE_DIR", "bundle"),
            max_bundle_size=int(max_size_str),
            allowed_extensions=extensions_str.split(",")
        )


@dataclass
class LoggingConfig:
    """Logging configuration settings."""
    level: str = "INFO"
    format: str = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    log_file: Optional[str] = None
    enable_structured_logging: bool = True
    
    @property
    def log_file_path(self) -> Optional[Path]:
        """Get log file as Path object."""
        return Path(self.log_file) if self.log_file else None
    
    @classmethod
    def from_env(cls) -> 'LoggingConfig':
        """Create config from environment variables."""
        return cls(
            level=os.getenv("UPDATER_LOG_LEVEL", "INFO"),
            format=os.getenv("UPDATER_LOG_FORMAT", "%(asctime)s - %(name)s - %(levelname)s - %(message)s"),
            log_file=os.getenv("UPDATER_LOG_FILE"),
            enable_structured_logging=os.getenv("UPDATER_STRUCTURED_LOGGING", "true").lower() == "true"
        )


@dataclass
class SecurityConfig:
    """Security configuration settings."""
    ssl_cert_file: str = "certs/server.crt"
    ssl_key_file: str = "certs/server.key"
    ssl_ca_file: str = "certs/ca.crt"
    cors_origins: List[str] = field(default_factory=lambda: ["*"])
    enable_auth: bool = False
    api_key: Optional[str] = None
    
    @property
    def ssl_cert_path(self) -> Path:
        """Get SSL certificate path."""
        return Path(self.ssl_cert_file)
    
    @property
    def ssl_key_path(self) -> Path:
        """Get SSL key path."""
        return Path(self.ssl_key_file)
    
    @property
    def ssl_ca_path(self) -> Path:
        """Get SSL CA path."""
        return Path(self.ssl_ca_file)
    
    @classmethod
    def from_env(cls) -> 'SecurityConfig':
        """Create config from environment variables."""
        cors_origins_str = os.getenv("UPDATER_CORS_ORIGINS", "*")
        
        return cls(
            ssl_cert_file=os.getenv("UPDATER_SSL_CERT", "certs/server.crt"),
            ssl_key_file=os.getenv("UPDATER_SSL_KEY", "certs/server.key"),
            ssl_ca_file=os.getenv("UPDATER_SSL_CA", "certs/ca.crt"),
            cors_origins=cors_origins_str.split(","),
            enable_auth=os.getenv("UPDATER_ENABLE_AUTH", "false").lower() == "true",
            api_key=os.getenv("UPDATER_API_KEY")
        )


@dataclass
class GuiConfig:
    """GUI configuration settings."""
    enable_gui: bool = True
    gui_path: str = "gui"
    static_files_path: str = "static"
    
    @property
    def gui_path_obj(self) -> Path:
        """Get GUI path as Path object."""
        return Path(self.gui_path)
    
    @property
    def static_path_obj(self) -> Path:
        """Get static files path as Path object."""
        return Path(self.static_files_path)
    
    @classmethod
    def from_env(cls) -> 'GuiConfig':
        """Create config from environment variables."""
        return cls(
            enable_gui=os.getenv("UPDATER_ENABLE_GUI", "true").lower() == "true",
            gui_path=os.getenv("UPDATER_GUI_PATH", "gui"),
            static_files_path=os.getenv("UPDATER_STATIC_PATH", "static")
        )


@dataclass
class ApplicationConfig:
    """Application configuration settings."""
    default_tenant: str = "default"
    max_deployments: int = 100
    deployment_timeout: int = 3600  # 1 hour
    enable_metrics: bool = False
    
    @classmethod
    def from_env(cls) -> 'ApplicationConfig':
        """Create config from environment variables."""
        return cls(
            default_tenant=os.getenv("UPDATER_DEFAULT_TENANT", "default"),
            max_deployments=int(os.getenv("UPDATER_MAX_DEPLOYMENTS", "100")),
            deployment_timeout=int(os.getenv("UPDATER_DEPLOYMENT_TIMEOUT", "3600")),
            enable_metrics=os.getenv("UPDATER_ENABLE_METRICS", "false").lower() == "true"
        )


@dataclass
class Config:
    """Main configuration class."""
    server: ServerConfig
    bundle: BundleConfig
    logging: LoggingConfig
    security: SecurityConfig
    gui: GuiConfig
    application: ApplicationConfig
    
    # Server info
    SERVER_NAME: str = "Updater Server"
    SERVER_VERSION: str = "0.3.0"
    SERVER_DESCRIPTION: str = "Refactored updater server implementation for OTA updates"
    
    @classmethod
    def from_env(cls) -> 'Config':
        """Create complete config from environment variables."""
        return cls(
            server=ServerConfig.from_env(),
            bundle=BundleConfig.from_env(),
            logging=LoggingConfig.from_env(),
            security=SecurityConfig.from_env(),
            gui=GuiConfig.from_env(),
            application=ApplicationConfig.from_env()
        )
    
    def validate(self) -> None:
        """Validate configuration settings."""
        # Validate ports
        if not (1 <= self.server.port <= 65535):
            raise ConfigurationError(f"Invalid port: {self.server.port}")
        if not (1 <= self.server.https_port <= 65535):
            raise ConfigurationError(f"Invalid HTTPS port: {self.server.https_port}")
        
        # Validate bundle directory
        if not self.bundle.bundle_dir:
            raise ConfigurationError("Bundle directory cannot be empty")
        
        # Validate logging level
        valid_levels = ["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
        if self.logging.level.upper() not in valid_levels:
            raise ConfigurationError(f"Invalid log level: {self.logging.level}")
        
        # Validate SSL files if HTTPS is enabled
        if self.server.enable_https:
            if not self.security.ssl_cert_path.exists():
                raise ConfigurationError(f"SSL certificate not found: {self.security.ssl_cert_path}")
            if not self.security.ssl_key_path.exists():
                raise ConfigurationError(f"SSL key not found: {self.security.ssl_key_path}")
    
    def create_directories(self) -> None:
        """Create necessary directories."""
        # Create bundle directory
        self.bundle.bundle_dir_path.mkdir(parents=True, exist_ok=True)
        
        # Create log directory if specified
        if self.logging.log_file_path:
            self.logging.log_file_path.parent.mkdir(parents=True, exist_ok=True)
        
        # Create GUI directories if GUI is enabled
        if self.gui.enable_gui:
            self.gui.gui_path_obj.mkdir(parents=True, exist_ok=True)
            self.gui.static_path_obj.mkdir(parents=True, exist_ok=True)


# Global config instance - load from environment
config = Config.from_env()

# Backward compatibility exports
HOST = config.server.host
PORT = config.server.port
HTTPS_PORT = config.server.https_port
DEBUG = config.server.debug
ENABLE_HTTPS = config.server.enable_https
EXTERNAL_HOST = config.server.external_host
EXTERNAL_URL = config.server.external_url

BUNDLE_DIR = config.bundle.bundle_dir
BUNDLE_DIR_PATH = config.bundle.bundle_dir_path

LOG_LEVEL = config.logging.level
LOG_FORMAT = config.logging.format

CORS_ORIGINS = config.security.cors_origins
DEFAULT_TENANT = config.application.default_tenant

SSL_CERT_FILE = config.security.ssl_cert_file
SSL_KEY_FILE = config.security.ssl_key_file
SSL_CA_FILE = config.security.ssl_ca_file
SSL_CERT_PATH = config.security.ssl_cert_path
SSL_KEY_PATH = config.security.ssl_key_path
SSL_CA_PATH = config.security.ssl_ca_path

ENABLE_GUI = config.gui.enable_gui
GUI_PATH = config.gui.gui_path
STATIC_FILES_PATH = config.gui.static_files_path
GUI_PATH_OBJ = config.gui.gui_path_obj
STATIC_PATH_OBJ = config.gui.static_path_obj

SERVER_NAME = config.SERVER_NAME
SERVER_VERSION = config.SERVER_VERSION
SERVER_DESCRIPTION = config.SERVER_DESCRIPTION