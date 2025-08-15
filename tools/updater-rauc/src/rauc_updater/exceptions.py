"""Custom exceptions for RAUC Updater."""


class RaucUpdaterError(Exception):
    """Base exception for RAUC Updater errors."""
    pass


class ConnectionError(RaucUpdaterError):
    """Raised when SSH connection fails."""
    pass


class AuthenticationError(ConnectionError):
    """Raised when SSH authentication fails."""
    pass


class TransferError(RaucUpdaterError):
    """Raised when file transfer fails."""
    pass


class InstallationError(RaucUpdaterError):
    """Raised when RAUC installation fails."""
    pass


class DBusError(RaucUpdaterError):
    """Raised when D-Bus operation fails."""
    pass


class HawkbitError(RaucUpdaterError):
    """Raised when Hawkbit operation fails."""
    pass


class NetworkConfigError(RaucUpdaterError):
    """Raised when network configuration fails."""
    pass


class ValidationError(RaucUpdaterError):
    """Raised when input validation fails."""
    pass