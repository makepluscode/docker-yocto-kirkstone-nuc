"""Network configuration and SSH setup commands."""

import subprocess
import sys
import os
from pathlib import Path
from typing import Optional

from rich.console import Console

from .config import DEFAULT_INTERFACE, DEFAULT_HOST_IP, DEFAULT_NETMASK
from .exceptions import NetworkConfigError
from .connection import copy_ssh_key

console = Console()


def setup_network_interface(interface: str, host_ip: str, netmask: str) -> bool:
    """Setup network interface configuration.
    
    Args:
        interface: Network interface name
        host_ip: IP address to assign
        netmask: Network mask
        
    Returns:
        True if successful, False otherwise
        
    Raises:
        NetworkConfigError: If configuration fails
    """
    try:
        # Check current IP
        result = subprocess.run(['ip', 'addr', 'show', interface], 
                              capture_output=True, text=True)
        
        current_ip = None
        if result.returncode == 0:
            for line in result.stdout.split('\n'):
                if 'inet ' in line:
                    current_ip = line.split()[1].split('/')[0]
                    break
        
        if current_ip != host_ip:
            console.print(f"Setting IP address for {interface}...")
            result = subprocess.run(['sudo', 'ifconfig', interface, host_ip, 
                                   'netmask', netmask, 'up'], 
                                  capture_output=True, text=True)
            if result.returncode != 0:
                raise NetworkConfigError(f"Failed to configure network interface: {result.stderr}")
            console.print(f"[green]✓ Network interface configured[/green]")
            return True
        else:
            console.print(f"[green]✓ Network interface already configured[/green]")
            return True
            
    except subprocess.CalledProcessError as e:
        raise NetworkConfigError(f"Network configuration failed: {e}")
    except Exception as e:
        raise NetworkConfigError(f"Unexpected error: {e}")


def setup_ssh_authentication(target_ip: str, target_user: str, port: int) -> bool:
    """Setup SSH authentication with key-based auth.
    
    Args:
        target_ip: Target device IP
        target_user: Target device username
        port: SSH port
        
    Returns:
        True if successful, False otherwise
    """
    try:
        ssh_dir = Path.home() / '.ssh'
        known_hosts = ssh_dir / 'known_hosts'
        ssh_key = ssh_dir / 'id_rsa'
        
        # Create SSH directory if needed
        ssh_dir.mkdir(mode=0o700, exist_ok=True)
        
        # Fix known_hosts permissions
        if known_hosts.exists() and not os.access(known_hosts, os.W_OK):
            console.print("Fixing SSH known_hosts permissions...")
            subprocess.run(['sudo', 'chown', f'{os.getlogin()}:{os.getlogin()}', str(known_hosts)])
            subprocess.run(['sudo', 'chmod', '600', str(known_hosts)])
        elif not known_hosts.exists():
            known_hosts.touch(mode=0o600)
        
        # Remove old host key
        console.print(f"Removing old host key for {target_ip}...")
        subprocess.run(['ssh-keygen', '-f', str(known_hosts), '-R', target_ip], 
                      capture_output=True)
        
        # Add new host key
        console.print(f"Adding new host key for {target_ip}...")
        result = subprocess.run(['ssh-keyscan', '-H', target_ip], 
                              capture_output=True, text=True)
        if result.returncode == 0:
            with open(known_hosts, 'a') as f:
                f.write(result.stdout)
        
        # Generate SSH key if needed
        if not ssh_key.exists():
            console.print("Generating SSH key pair...")
            subprocess.run(['ssh-keygen', '-t', 'rsa', '-b', '2048', 
                          '-f', str(ssh_key), '-N', ''])
            console.print(f"[green]✓ SSH key generated[/green]")
        
        # Test SSH key authentication
        console.print("Testing SSH key authentication...")
        result = subprocess.run(['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=5',
                               f'{target_user}@{target_ip}', 'exit'], 
                              capture_output=True)
        
        if result.returncode == 0:
            console.print("[bold green]✓ SSH key authentication successful![/bold green]")
            return True
        else:
            console.print("[yellow]Setting up SSH key authentication...[/yellow]")
            console.print("You may need to enter the target device password once.")
            
            # Try ssh-copy-id
            result = subprocess.run(['ssh-copy-id', '-o', 'ConnectTimeout=10',
                                   f'{target_user}@{target_ip}'])
            
            if result.returncode == 0:
                console.print("[bold green]✓ SSH key setup successful![/bold green]")
                return True
            else:
                console.print("[red]✗ SSH key setup failed[/red]")
                return False
                
    except Exception as e:
        console.print(f"[red]✗ SSH setup error: {e}[/red]")
        return False


def connect_command(interface: str, host_ip: str, netmask: str, target_ip: str, 
                   target_user: str, skip_network: bool):
    """Setup network connection and SSH authentication to target device."""
    
    console.print("[bold blue]Setting up connection to target device...[/bold blue]")
    console.print(f"Interface: [cyan]{interface}[/cyan]")
    console.print(f"Host IP: [cyan]{host_ip}[/cyan]")
    console.print(f"Target: [cyan]{target_user}@{target_ip}[/cyan]")
    console.print()
    
    try:
        # Step 1: Network configuration
        if not skip_network:
            console.print("[bold blue]Step 1: Configuring network interface...[/bold blue]")
            setup_network_interface(interface, host_ip, netmask)
        
        # Step 2: SSH setup
        console.print("[bold blue]Step 2: Setting up SSH authentication...[/bold blue]")
        if not setup_ssh_authentication(target_ip, target_user, 22):
            sys.exit(1)
        
        # Step 3: Final connection test
        console.print("[bold blue]Step 3: Testing final connection...[/bold blue]")
        result = subprocess.run(['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=5',
                               f'{target_user}@{target_ip}', 'exit'], 
                              capture_output=True)
        
        if result.returncode == 0:
            console.print("[bold green]✓ Connection setup completed successfully![/bold green]")
            console.print(f"You can now use other arcro commands with --host {target_ip}")
        else:
            console.print("[red]✗ Connection test failed[/red]")
            sys.exit(1)
            
    except KeyboardInterrupt:
        console.print("[yellow]\\nSetup cancelled by user[/yellow]")
        sys.exit(1)
    except Exception as e:
        console.print(f"[red]✗ Connection setup failed: {e}[/red]")
        sys.exit(1)