"""Logging utilities for updater server."""

import logging
import sys
from pathlib import Path
from typing import Optional


def setup_logging(
    level: str = "INFO",
    format_string: Optional[str] = None,
    log_file: Optional[Path] = None
) -> None:
    """Setup logging configuration.
    
    Args:
        level: Logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
        format_string: Custom format string
        log_file: Optional log file path
    """
    if format_string is None:
        format_string = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    
    # Configure root logger
    logging.basicConfig(
        level=getattr(logging, level.upper()),
        format=format_string,
        handlers=[
            logging.StreamHandler(sys.stdout)
        ]
    )
    
    # Add file handler if specified
    if log_file:
        log_file.parent.mkdir(parents=True, exist_ok=True)
        file_handler = logging.FileHandler(log_file)
        file_handler.setFormatter(logging.Formatter(format_string))
        logging.getLogger().addHandler(file_handler)
    
    # Set specific loggers
    logging.getLogger("uvicorn.access").setLevel(logging.WARNING)
    logging.getLogger("fastapi").setLevel(logging.WARNING)


def get_logger(name: str) -> logging.Logger:
    """Get a logger with the specified name."""
    return logging.getLogger(name)


class StructuredLogger:
    """Structured logger for better log organization."""
    
    def __init__(self, name: str):
        self.logger = logging.getLogger(name)
    
    def log_request(self, method: str, path: str, **kwargs):
        """Log HTTP request."""
        extra_info = " ".join(f"{k}={v}" for k, v in kwargs.items())
        self.logger.info(f"REQUEST {method} {path} {extra_info}")
    
    def log_response(self, status_code: int, path: str, **kwargs):
        """Log HTTP response."""
        extra_info = " ".join(f"{k}={v}" for k, v in kwargs.items())
        self.logger.info(f"RESPONSE {status_code} {path} {extra_info}")
    
    def log_deployment(self, action: str, execution_id: str, **kwargs):
        """Log deployment action."""
        extra_info = " ".join(f"{k}={v}" for k, v in kwargs.items())
        self.logger.info(f"DEPLOYMENT {action} {execution_id} {extra_info}")
    
    def log_feedback(self, controller_id: str, status: str, progress: int, **kwargs):
        """Log device feedback."""
        extra_info = " ".join(f"{k}={v}" for k, v in kwargs.items())
        self.logger.info(f"FEEDBACK {controller_id} {status} {progress}% {extra_info}")
    
    def log_error(self, error: Exception, context: str = "", **kwargs):
        """Log error with context."""
        extra_info = " ".join(f"{k}={v}" for k, v in kwargs.items())
        self.logger.error(f"ERROR {context} {type(error).__name__}: {error} {extra_info}")