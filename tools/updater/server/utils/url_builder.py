"""URL builder utility for constructing server URLs."""

from typing import Optional
from urllib.parse import urljoin


class URLBuilder:
    """Utility for building server URLs consistently."""
    
    def __init__(self, base_url: str, protocol: str = "http", host: str = "localhost", port: int = 8080):
        """Initialize URL builder.
        
        Args:
            base_url: Complete base URL (takes precedence if provided)
            protocol: Protocol (http/https)
            host: Server host
            port: Server port
        """
        if base_url:
            self.base_url = base_url.rstrip('/')
        else:
            self.base_url = f"{protocol}://{host}:{port}"
    
    def build_download_url(self, filename: str) -> str:
        """Build download URL for a bundle file."""
        return f"{self.base_url}/download/{filename}"
    
    def build_poll_url(self, tenant: str, controller_id: str) -> str:
        """Build poll URL for a device."""
        return f"{self.base_url}/{tenant}/controller/v1/{controller_id}"
    
    def build_feedback_url(self, tenant: str, controller_id: str, execution_id: str) -> str:
        """Build feedback URL for a device deployment."""
        return f"{self.base_url}/{tenant}/controller/v1/{controller_id}/deploymentBase/{execution_id}/feedback"
    
    def build_admin_url(self, path: str = "") -> str:
        """Build admin API URL."""
        return f"{self.base_url}/admin/{path.lstrip('/')}"
    
    @classmethod
    def from_config(cls, config) -> 'URLBuilder':
        """Create URL builder from configuration."""
        # Determine external URL for clients
        if hasattr(config, 'EXTERNAL_URL') and config.EXTERNAL_URL:
            return cls(base_url=config.EXTERNAL_URL)
        
        # Build URL from config
        protocol = "https" if getattr(config, 'ENABLE_HTTPS', False) else "http"
        host = getattr(config, 'EXTERNAL_HOST', getattr(config, 'HOST', 'localhost'))
        
        if protocol == "https":
            port = getattr(config, 'HTTPS_PORT', 8443)
        else:
            port = getattr(config, 'PORT', 8080)
        
        return cls(base_url="", protocol=protocol, host=host, port=port)