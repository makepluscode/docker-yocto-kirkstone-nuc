"""Validation utilities for updater server."""

import re
from typing import Optional


def validate_tenant(tenant: str) -> bool:
    """Validate tenant name.
    
    Args:
        tenant: Tenant name to validate
        
    Returns:
        bool: True if valid, False otherwise
    """
    if not tenant:
        return False
    
    # Allow alphanumeric, hyphens, and underscores
    pattern = r'^[a-zA-Z0-9_-]+$'
    return bool(re.match(pattern, tenant)) and len(tenant) <= 64


def validate_controller_id(controller_id: str) -> bool:
    """Validate controller ID.
    
    Args:
        controller_id: Controller ID to validate
        
    Returns:
        bool: True if valid, False otherwise
    """
    if not controller_id:
        return False
    
    # Allow alphanumeric, hyphens, underscores, and dots
    pattern = r'^[a-zA-Z0-9._-]+$'
    return bool(re.match(pattern, controller_id)) and len(controller_id) <= 128


def validate_execution_id(execution_id: str) -> bool:
    """Validate execution ID.
    
    Args:
        execution_id: Execution ID to validate
        
    Returns:
        bool: True if valid, False otherwise
    """
    if not execution_id:
        return False
    
    # Allow alphanumeric, hyphens, and underscores
    pattern = r'^[a-zA-Z0-9_-]+$'
    return bool(re.match(pattern, execution_id)) and len(execution_id) <= 128


def validate_filename(filename: str, allowed_extensions: Optional[list] = None) -> bool:
    """Validate filename.
    
    Args:
        filename: Filename to validate
        allowed_extensions: List of allowed file extensions
        
    Returns:
        bool: True if valid, False otherwise
    """
    if not filename:
        return False
    
    # Basic filename validation
    if '..' in filename or '/' in filename or '\\' in filename:
        return False
    
    if allowed_extensions:
        return any(filename.lower().endswith(ext.lower()) for ext in allowed_extensions)
    
    return True


def sanitize_input(value: str, max_length: int = 256) -> str:
    """Sanitize input string.
    
    Args:
        value: Input string to sanitize
        max_length: Maximum allowed length
        
    Returns:
        str: Sanitized string
    """
    if not value:
        return ""
    
    # Remove potentially dangerous characters
    sanitized = re.sub(r'[<>"&\']', '', value)
    
    # Truncate to max length
    return sanitized[:max_length].strip()