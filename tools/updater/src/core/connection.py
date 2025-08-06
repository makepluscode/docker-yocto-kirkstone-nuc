"""SSH connection management for RAUC updater."""

import subprocess
import time
from pathlib import Path
from typing import Optional, Tuple

import paramiko
from pydantic import BaseModel
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn

console = Console()


class ConnectionConfig(BaseModel):
    """Configuration for SSH connection."""
    
    host: str = "192.168.1.100"  # Default from connect.sh
    port: int = 22
    username: str = "root"  # Default from connect.sh
    password: Optional[str] = None
    key_filename: Optional[str] = None
    timeout: int = 30
    
    class Config:
        arbitrary_types_allowed = True


class SSHConnection:
    """Manages SSH connection to target device."""
    
    def __init__(self, config: ConnectionConfig):
        self.config = config
        self.client: Optional[paramiko.SSHClient] = None
        self._connected = False
    
    def connect(self) -> bool:
        """Establish SSH connection to target device."""
        try:
            self.client = paramiko.SSHClient()
            self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            
            console.print(f"[blue]Connecting to {self.config.username}@{self.config.host}:{self.config.port}...[/blue]")
            
            # Connection parameters
            connect_kwargs = {
                "hostname": self.config.host,
                "port": self.config.port,
                "username": self.config.username,
                "timeout": self.config.timeout,
            }
            
            # Add authentication method
            if self.config.key_filename:
                connect_kwargs["key_filename"] = self.config.key_filename
            elif self.config.password:
                connect_kwargs["password"] = self.config.password
            else:
                # Try to use default SSH keys
                ssh_dir = Path.home() / ".ssh"
                for key_file in ["id_rsa", "id_ecdsa", "id_ed25519"]:
                    key_path = ssh_dir / key_file
                    if key_path.exists():
                        connect_kwargs["key_filename"] = str(key_path)
                        break
            
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                console=console
            ) as progress:
                task = progress.add_task("Establishing connection...", total=None)
                self.client.connect(**connect_kwargs)
                progress.update(task, completed=True)
            
            self._connected = True
            console.print("[green]✓ Connected successfully[/green]")
            return True
            
        except paramiko.AuthenticationException:
            console.print("[red]✗ Authentication failed[/red]")
            return False
        except paramiko.SSHException as e:
            console.print(f"[red]✗ SSH connection failed: {e}[/red]")
            return False
        except Exception as e:
            console.print(f"[red]✗ Connection error: {e}[/red]")
            return False
    
    def disconnect(self):
        """Close SSH connection."""
        if self.client:
            self.client.close()
            self._connected = False
            console.print("[yellow]Connection closed[/yellow]")
    
    def is_connected(self) -> bool:
        """Check if connection is active."""
        return self._connected and self.client is not None
    
    def execute_command(self, command: str, timeout: int = 300) -> Tuple[int, str, str]:
        """Execute command on remote host.
        
        Returns:
            Tuple of (exit_code, stdout, stderr)
        """
        if not self.is_connected():
            raise RuntimeError("Not connected to remote host")
        
        console.print(f"[cyan]Executing: {command}[/cyan]")
        
        stdin, stdout, stderr = self.client.exec_command(command, timeout=timeout)
        
        # Wait for command to complete
        exit_code = stdout.channel.recv_exit_status()
        stdout_text = stdout.read().decode('utf-8')
        stderr_text = stderr.read().decode('utf-8')
        
        if exit_code == 0:
            console.print("[green]✓ Command completed successfully[/green]")
        else:
            console.print(f"[red]✗ Command failed with exit code {exit_code}[/red]")
            if stderr_text:
                console.print(f"[red]Error: {stderr_text.strip()}[/red]")
        
        return exit_code, stdout_text, stderr_text
    
    def test_rauc_availability(self) -> bool:
        """Test if RAUC is available on target system."""
        try:
            exit_code, stdout, stderr = self.execute_command("which rauc")
            if exit_code == 0:
                # Get RAUC version
                exit_code, version_output, _ = self.execute_command("rauc --version")
                if exit_code == 0:
                    version_line = version_output.strip().split('\n')[0]
                    console.print(f"[green]✓ RAUC found: {version_line}[/green]")
                    return True
            
            console.print("[red]✗ RAUC not found on target system[/red]")
            return False
            
        except Exception as e:
            console.print(f"[red]✗ Failed to check RAUC availability: {e}[/red]")
            return False
    
    def get_disk_space(self, path: str = "/") -> Optional[dict]:
        """Get available disk space at specified path."""
        try:
            exit_code, stdout, stderr = self.execute_command(f"df -h {path}")
            if exit_code == 0:
                lines = stdout.strip().split('\n')
                if len(lines) >= 2:
                    fields = lines[1].split()
                    if len(fields) >= 6:
                        return {
                            "filesystem": fields[0],
                            "size": fields[1],
                            "used": fields[2],
                            "available": fields[3],
                            "use_percent": fields[4],
                            "mounted_on": fields[5]
                        }
            return None
        except Exception as e:
            console.print(f"[yellow]Warning: Could not get disk space info: {e}[/yellow]")
            return None
    
    def __enter__(self):
        """Context manager entry."""
        if not self.connect():
            raise RuntimeError("Failed to establish SSH connection")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()


def test_ssh_connection(host: str, username: str = "root", port: int = 22, timeout: int = 5) -> bool:
    """Test if SSH connection works without password (key-based auth)."""
    try:
        cmd = [
            "ssh", 
            "-o", "StrictHostKeyChecking=no",
            "-o", "PasswordAuthentication=no",
            "-o", f"ConnectTimeout={timeout}",
            "-p", str(port),
            f"{username}@{host}",
            "echo", "test"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout+2)
        return result.returncode == 0
        
    except (subprocess.TimeoutExpired, Exception):
        return False


def copy_ssh_key(host: str, username: str = "root", port: int = 22, password: str = "root") -> bool:
    """Copy SSH public key to target device with multiple fallback methods."""
    
    # First check if SSH key authentication already works
    console.print(f"[blue]Checking SSH key authentication for {username}@{host}:{port}...[/blue]")
    if test_ssh_connection(host, username, port):
        console.print("[green]✓ SSH key authentication already working[/green]")
        return True
    
    console.print(f"[blue]Setting up SSH key authentication for {username}@{host}:{port}...[/blue]")
    
    # Method 1: Try ssh-copy-id with sshpass
    try:
        if subprocess.run(["which", "sshpass"], capture_output=True).returncode == 0:
            console.print("[cyan]Method 1: Using sshpass + ssh-copy-id[/cyan]")
            
            cmd = [
                "sshpass", "-p", password,
                "ssh-copy-id", "-o", "StrictHostKeyChecking=no",
                "-p", str(port),
                f"{username}@{host}"
            ]
            
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                console=console
            ) as progress:
                task = progress.add_task("Copying SSH key (method 1)...", total=None)
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
                progress.update(task, completed=True)
            
            if result.returncode == 0:
                console.print("[green]✓ SSH key copied successfully (method 1)[/green]")
                return True
            elif "already exist" in result.stderr.lower():
                console.print("[green]✓ SSH key already exists on target[/green]")
                return True
            else:
                console.print(f"[yellow]⚠ Method 1 failed: {result.stderr.strip()}[/yellow]")
        else:
            console.print("[yellow]⚠ sshpass not available, skipping method 1[/yellow]")
            
    except (subprocess.TimeoutExpired, FileNotFoundError, Exception) as e:
        console.print(f"[yellow]⚠ Method 1 error: {e}[/yellow]")
    
    # Method 2: Try forced ssh-copy-id
    try:
        console.print("[cyan]Method 2: Forced ssh-copy-id with sshpass[/cyan]")
        cmd = [
            "sshpass", "-p", password,
            "ssh-copy-id", "-f", "-o", "StrictHostKeyChecking=no",
            "-p", str(port),
            f"{username}@{host}"
        ]
        
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console
        ) as progress:
            task = progress.add_task("Copying SSH key (method 2)...", total=None)
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            progress.update(task, completed=True)
        
        if result.returncode == 0:
            console.print("[green]✓ SSH key copied successfully (method 2)[/green]")
            return True
        else:
            console.print(f"[yellow]⚠ Method 2 failed: {result.stderr.strip()}[/yellow]")
            
    except (subprocess.TimeoutExpired, Exception) as e:
        console.print(f"[yellow]⚠ Method 2 error: {e}[/yellow]")
    
    # Method 3: Manual key copying using SSH and echo
    try:
        console.print("[cyan]Method 3: Manual public key copying[/cyan]")
        
        # Find SSH public key
        ssh_dir = Path.home() / ".ssh"
        public_key_content = None
        
        for key_type in ["id_ed25519.pub", "id_ecdsa.pub", "id_rsa.pub"]:
            pub_key_path = ssh_dir / key_type
            if pub_key_path.exists():
                public_key_content = pub_key_path.read_text().strip()
                console.print(f"[blue]Found public key: {key_type}[/blue]")
                break
        
        if not public_key_content:
            console.print("[yellow]⚠ No SSH public key found[/yellow]")
        else:
            # Create authorized_keys directory and add key
            setup_cmd = f"""
                mkdir -p ~/.ssh && 
                chmod 700 ~/.ssh && 
                echo '{public_key_content}' >> ~/.ssh/authorized_keys && 
                chmod 600 ~/.ssh/authorized_keys && 
                sort ~/.ssh/authorized_keys | uniq > ~/.ssh/authorized_keys.tmp && 
                mv ~/.ssh/authorized_keys.tmp ~/.ssh/authorized_keys
            """
            
            cmd = [
                "sshpass", "-p", password,
                "ssh", "-o", "StrictHostKeyChecking=no",
                "-p", str(port),
                f"{username}@{host}",
                setup_cmd
            ]
            
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                console=console
            ) as progress:
                task = progress.add_task("Copying SSH key (method 3)...", total=None)
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
                progress.update(task, completed=True)
            
            if result.returncode == 0:
                console.print("[green]✓ SSH key copied successfully (method 3)[/green]")
                return True
            else:
                console.print(f"[yellow]⚠ Method 3 failed: {result.stderr.strip()}[/yellow]")
                
    except Exception as e:
        console.print(f"[yellow]⚠ Method 3 error: {e}[/yellow]")
    
    # Final verification test
    console.print("[cyan]Final verification: Testing SSH key authentication...[/cyan]")
    if test_ssh_connection(host, username, port):
        console.print("[green]✓ SSH key authentication now working![/green]")
        return True
    
    # All methods failed
    console.print("[red]✗ All SSH key setup methods failed[/red]")
    console.print("[yellow]You may need to manually setup SSH keys or check network connectivity[/yellow]")
    return False


def test_connection(host: str = "192.168.1.100", username: str = "root", port: int = 22) -> bool:
    """Test SSH connection to target device."""
    config = ConnectionConfig(host=host, username=username, port=port)
    
    try:
        with SSHConnection(config) as conn:
            # Test basic connectivity
            if not conn.test_rauc_availability():
                return False
            
            # Show system info
            exit_code, output, _ = conn.execute_command("uname -a")
            if exit_code == 0:
                console.print(f"[blue]System: {output.strip()}[/blue]")
            
            # Show disk space
            disk_info = conn.get_disk_space("/")
            if disk_info:
                console.print(f"[blue]Disk space: {disk_info['available']} available[/blue]")
            
            return True
            
    except Exception as e:
        console.print(f"[red]Connection test failed: {e}[/red]")
        return False