"""CLI command implementations for RAUC Updater."""

import sys
import subprocess
import os
from pathlib import Path
from typing import Optional

import click
from rich.console import Console
from rich.panel import Panel

from .config import (
    DEFAULT_HOST, DEFAULT_USER, DEFAULT_PORT, DEFAULT_INTERFACE,
    DEFAULT_HOST_IP, DEFAULT_NETMASK, DEFAULT_REMOTE_PATH, DEFAULT_TIMEOUT,
    DBUS_DEST, DBUS_PATH, DBUS_INTERFACE, SSH_ERROR_KEYWORDS, HAWKBIT_DEFAULTS
)
from .exceptions import ConnectionError, DBusError, HawkbitError, NetworkConfigError
from .connection import ConnectionConfig, SSHConnection
from .installer import InstallConfig, install_rauc_bundle
from .transfer import TransferConfig, upload_bundle
from .utils import validate_bundle_file, console


def pure_dbus_status_command():
    """Get RAUC status via pure D-Bus interface (no SSH)."""
    
    console.print("[bold blue]RAUC Pure D-Bus Status:[/bold blue]")
    
    try:
        # Use local D-Bus to get status
        result = subprocess.run([
            'dbus-send', '--system', '--print-reply',
            f'--dest={DBUS_DEST}', DBUS_PATH,
            f'{DBUS_INTERFACE}.GetSlotStatus'
        ], capture_output=True, text=True, timeout=10)
        
        exit_code, output, stderr = result.returncode, result.stdout, result.stderr
        
        if exit_code == 0:
            console.print(Panel(
                output.strip(),
                title="RAUC Pure D-Bus Status",
                border_style="green"
            ))
        else:
            console.print(f"[red]Pure D-Bus error: {stderr.strip()}[/red]")
            
    except Exception as e:
        console.print(f"[red]✗ Failed to get pure D-Bus status: {e}[/red]")
        sys.exit(1)


def pure_dbus_update_command(bundle_path: Path):
    """Update using pure D-Bus interface (no SSH)."""
    
    console.print(f"[bold green]Starting Pure D-Bus RAUC Update[/bold green]")
    console.print(f"Bundle: [cyan]{bundle_path}[/cyan]")
    console.print()
    
    try:
        # Test D-Bus connection first
        console.print("[bold blue]Step 1: Testing D-Bus connection...[/bold blue]")
        result = subprocess.run([
            'dbus-send', '--system', '--print-reply',
            f'--dest={DBUS_DEST}', DBUS_PATH,
            'org.freedesktop.DBus.Introspectable.Introspect'
        ], capture_output=True, timeout=10)
        
        if result.returncode != 0:
            console.print("[red]✗ RAUC D-Bus service not available[/red]")
            sys.exit(1)
        
        console.print("[green]✓ D-Bus connection successful[/green]")
        
        # Install bundle via D-Bus
        console.print("[bold blue]Step 2: Installing via D-Bus...[/bold blue]")
        
        dbus_cmd = [
            'dbus-send', '--system', '--print-reply',
            f'--dest={DBUS_DEST}', DBUS_PATH,
            f'{DBUS_INTERFACE}.Install',
            f'string:"{bundle_path}"'
        ]
        
        console.print(f"D-Bus command: {' '.join(dbus_cmd)}")
        result = subprocess.run(dbus_cmd, capture_output=True, timeout=300)
        
        if result.returncode == 0:
            console.print("[green]✓ D-Bus installation completed[/green]")
            console.print(Panel(
                result.stdout.decode().strip(),
                title="D-Bus Installation Output",
                border_style="green"
            ))
            console.print()
            console.print("[bold green]✓ Pure D-Bus update completed successfully![/bold green]")
        else:
            error_msg = result.stderr.decode() if result.stderr else "Unknown D-Bus error"
            console.print(f"[red]✗ D-Bus installation failed: {error_msg}[/red]")
            if result.stdout:
                console.print(f"D-Bus output: {result.stdout.decode()}")
            sys.exit(1)
            
    except subprocess.TimeoutExpired:
        console.print("[red]✗ D-Bus operation timed out[/red]")
        sys.exit(1)
    except Exception as e:
        console.print(f"[red]✗ Pure D-Bus update failed: {e}[/red]")
        sys.exit(1)

def test_connection_command(host: str, user: str, port: int, password: Optional[str], key_file: Optional[str]):
    """Test connection to target device."""
    
    console.print(f"[bold blue]Testing connection to {user}@{host}:{port}[/bold blue]")
    
    config = ConnectionConfig(
        host=host,
        port=port,
        username=user,
        password=password,
        key_filename=key_file
    )
    
    try:
        with SSHConnection(config) as ssh_conn:
            if ssh_conn.test_rauc_availability():
                console.print("[bold green]✓ Connection test successful![/bold green]")
                
                # Show additional info
                status = ssh_conn.get_disk_space("/")
                if status:
                    console.print(f"Root filesystem: {status['available']} available")
                
                status = ssh_conn.get_disk_space("/tmp")
                if status:
                    console.print(f"/tmp directory: {status['available']} available")
                    
            else:
                console.print("[red]✗ Connection test failed[/red]")
                sys.exit(1)
                
    except Exception as e:
        console.print(f"[red]✗ Connection failed: {e}[/red]")
        sys.exit(1)


def status_command(host: str, user: str, port: int, password: Optional[str], key_file: Optional[str]):
    """Show RAUC status on target device."""
    
    config = ConnectionConfig(
        host=host,
        port=port,
        username=user,
        password=password,
        key_filename=key_file
    )
    
    try:
        with SSHConnection(config) as ssh_conn:
            console.print("[bold blue]RAUC Status:[/bold blue]")
            
            exit_code, output, stderr = ssh_conn.execute_command("rauc status")
            
            if exit_code == 0:
                console.print(Panel(
                    output.strip(),
                    title="RAUC System Status",
                    border_style="green"
                ))
            else:
                console.print(f"[red]Error getting status: {stderr.strip()}[/red]")
                
    except Exception as e:
        console.print(f"[red]✗ Failed to get status: {e}[/red]")
        sys.exit(1)


def dbus_status_command(host: str, user: str, port: int, password: Optional[str], key_file: Optional[str]):
    """Get RAUC status via D-Bus interface."""
    
    config = ConnectionConfig(
        host=host,
        port=port,
        username=user,
        password=password,
        key_filename=key_file
    )
    
    try:
        with SSHConnection(config) as ssh_conn:
            console.print("[bold blue]RAUC D-Bus Status:[/bold blue]")
            
            # Use D-Bus to get status
            exit_code, output, stderr = ssh_conn.execute_command(
                f"dbus-send --system --print-reply --dest={DBUS_DEST} "
                f"{DBUS_PATH} {DBUS_INTERFACE}.GetSlotStatus"
            )
            
            if exit_code == 0:
                console.print(Panel(
                    output.strip(),
                    title="RAUC D-Bus Status",
                    border_style="green"
                ))
            else:
                console.print(f"[red]D-Bus error: {stderr.strip()}[/red]")
                # Fallback to regular status
                console.print("[yellow]Falling back to regular status...[/yellow]")
                status_command(host, user, port, password, key_file)
                
    except Exception as e:
        console.print(f"[red]✗ Failed to get D-Bus status: {e}[/red]")
        sys.exit(1)


def cleanup_command(host: str, user: str, port: int, password: Optional[str], key_file: Optional[str]):
    """Clean up temporary files on target device."""
    
    config = ConnectionConfig(
        host=host,
        port=port,
        username=user,
        password=password,
        key_filename=key_file
    )
    
    try:
        with SSHConnection(config) as ssh_conn:
            console.print("[bold blue]Cleaning up temporary files...[/bold blue]")
            
            # List existing bundle files
            from .transfer import FileTransfer, TransferConfig
            transfer_config = TransferConfig(remote_path="/tmp")
            transfer = FileTransfer(ssh_conn, transfer_config)
            
            files = transfer.list_remote_files("*.raucb")
            
            if files:
                console.print(f"Found {len(files)} bundle files:")
                for file_info in files:
                    console.print(f"  - {file_info['name']} ({file_info['size']})")
                
                if click.confirm("Remove these files?"):
                    for file_info in files:
                        transfer.remove_remote_file(file_info['name'])
                    console.print("[green]✓ Cleanup completed[/green]")
                else:
                    console.print("[yellow]Cleanup cancelled[/yellow]")
            else:
                console.print("[green]No bundle files found[/green]")
                
    except Exception as e:
        console.print(f"[red]✗ Cleanup failed: {e}[/red]")
        sys.exit(1)