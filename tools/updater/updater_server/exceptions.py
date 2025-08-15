"""Custom exceptions for updater server."""


class UpdaterServerError(Exception):
    """Base exception for updater server errors."""
    pass


class DeploymentError(UpdaterServerError):
    """Base exception for deployment-related errors."""
    pass


class DeploymentNotFoundError(DeploymentError):
    """Raised when a deployment is not found."""
    pass


class DeploymentCreationError(DeploymentError):
    """Raised when deployment creation fails."""
    pass


class BundleError(UpdaterServerError):
    """Base exception for bundle-related errors."""
    pass


class BundleNotFoundError(BundleError):
    """Raised when a bundle is not found."""
    pass


class BundleValidationError(BundleError):
    """Raised when bundle validation fails."""
    pass


class ValidationError(UpdaterServerError):
    """Raised when input validation fails."""
    pass


class ConfigurationError(UpdaterServerError):
    """Raised when there's a configuration error."""
    pass