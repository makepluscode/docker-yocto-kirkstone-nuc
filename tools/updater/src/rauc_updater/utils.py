"""Utility functions for RAUC updater."""

import logging
import sys
from pathlib import Path
from typing import Optional

from rich.console import Console
from rich.logging import RichHandler

console = Console()


def setup_logging(verbose: bool = False, log_file: Optional[str] = None) -> logging.Logger:
    """Setup logging configuration.
    
    Args:
        verbose: Enable verbose logging
        log_file: Optional log file path
        
    Returns:
        Configured logger instance
    """
    
    # Create logger
    logger = logging.getLogger("rauc_updater")
    logger.setLevel(logging.DEBUG if verbose else logging.INFO)
    
    # Clear existing handlers
    logger.handlers.clear()
    
    # Console handler with Rich formatting
    console_handler = RichHandler(
        console=console,
        show_time=False,
        show_path=False,
        rich_tracebacks=True
    )
    console_handler.setLevel(logging.DEBUG if verbose else logging.INFO)
    
    console_format = "%(message)s"
    console_handler.setFormatter(logging.Formatter(console_format))
    logger.addHandler(console_handler)
    
    # File handler if specified
    if log_file:
        log_path = Path(log_file)
        log_path.parent.mkdir(parents=True, exist_ok=True)
        
        file_handler = logging.FileHandler(log_path)
        file_handler.setLevel(logging.DEBUG)
        
        file_format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
        file_handler.setFormatter(logging.Formatter(file_format))
        logger.addHandler(file_handler)
    
    return logger


def validate_bundle_file(bundle_path: Path) -> bool:
    """Validate RAUC bundle file.
    
    Args:
        bundle_path: Path to bundle file
        
    Returns:
        True if valid, False otherwise
    """
    if not bundle_path.exists():
        console.print(f"[red]✗ Bundle file not found: {bundle_path}[/red]")
        return False
    
    if not bundle_path.is_file():
        console.print(f"[red]✗ Path is not a file: {bundle_path}[/red]")
        return False
    
    if not bundle_path.suffix == '.raucb':
        console.print(f"[yellow]Warning: File doesn't have .raucb extension: {bundle_path}[/yellow]")
    
    # Check file size
    size = bundle_path.stat().st_size
    if size == 0:
        console.print(f"[red]✗ Bundle file is empty: {bundle_path}[/red]")
        return False
    
    # Basic size check (bundles are typically > 1MB)
    if size < 1024 * 1024:
        console.print(f"[yellow]Warning: Bundle file seems small ({size} bytes): {bundle_path}[/yellow]")
    
    console.print(f"[green]✓ Bundle file validated: {bundle_path} ({format_bytes(size)})[/green]")
    return True


def format_bytes(bytes_count: int) -> str:
    """Format bytes to human readable string."""
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if bytes_count < 1024.0:
            return f"{bytes_count:.1f}{unit}"
        bytes_count /= 1024.0
    return f"{bytes_count:.1f}PB"


def format_duration(seconds: float) -> str:
    """Format duration in seconds to human readable string."""
    if seconds < 60:
        return f"{seconds:.1f}s"
    elif seconds < 3600:
        minutes = int(seconds // 60)
        secs = int(seconds % 60)
        return f"{minutes}m {secs}s"
    else:
        hours = int(seconds // 3600)
        minutes = int((seconds % 3600) // 60)
        return f"{hours}h {minutes}m"


def confirm_action(message: str, default: bool = True) -> bool:
    """Confirm action with user.
    
    Args:
        message: Confirmation message
        default: Default choice
        
    Returns:
        User's choice
    """
    try:
        import click
        return click.confirm(message, default=default)
    except ImportError:
        # Fallback for testing
        response = input(f"{message} {'[Y/n]' if default else '[y/N]'}: ").strip().lower()
        if not response:
            return default
        return response in ('y', 'yes', '1', 'true')