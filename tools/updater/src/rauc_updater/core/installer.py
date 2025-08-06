"""RAUC installation execution and monitoring."""

import re
import time
from typing import Callable, Dict, Optional, Tuple

from pydantic import BaseModel
from rich.console import Console
from rich.live import Live
from rich.panel import Panel
from rich.progress import BarColumn, Progress, SpinnerColumn, TextColumn
from rich.table import Table

from .connection import SSHConnection

console = Console()


class InstallConfig(BaseModel):
    """Configuration for RAUC installation."""
    
    timeout: int = 600  # 10 minutes default timeout
    progress_interval: float = 1.0  # Progress check interval in seconds
    ignore_compatible: bool = False
    force_install: bool = False
    
    class Config:
        arbitrary_types_allowed = True


class InstallProgress(BaseModel):
    """RAUC installation progress information."""
    
    percentage: int = 0
    message: str = ""
    operation: str = ""
    slot: str = ""
    image: str = ""
    completed: bool = False
    error: Optional[str] = None
    
    class Config:
        arbitrary_types_allowed = True


class RaucInstaller:
    """Handles RAUC bundle installation with progress monitoring."""
    
    def __init__(self, ssh_connection: SSHConnection, config: Optional[InstallConfig] = None):
        self.ssh_connection = ssh_connection
        self.config = config or InstallConfig()
        self._install_running = False
    
    def install_bundle(
        self,
        remote_bundle_path: str,
        progress_callback: Optional[Callable[[InstallProgress], None]] = None
    ) -> bool:
        """Install RAUC bundle on target device.
        
        Args:
            remote_bundle_path: Path to bundle file on remote device
            progress_callback: Optional callback for progress updates
            
        Returns:
            True if installation successful, False otherwise
        """
        console.print(f"[blue]Installing RAUC bundle: {remote_bundle_path}[/blue]")
        
        # Verify bundle exists
        if not self._verify_bundle_exists(remote_bundle_path):
            return False
        
        # Check RAUC status before installation
        if not self._check_rauc_status():
            return False
        
        # Build install command
        install_cmd = self._build_install_command(remote_bundle_path)
        console.print(f"[cyan]Command: {install_cmd}[/cyan]")
        
        # Execute installation with progress monitoring
        return self._execute_with_progress(install_cmd, progress_callback)
    
    def _verify_bundle_exists(self, remote_bundle_path: str) -> bool:
        """Verify that the bundle file exists on remote device."""
        try:
            exit_code, _, stderr = self.ssh_connection.execute_command(
                f"test -f {remote_bundle_path}"
            )
            
            if exit_code == 0:
                # Get bundle info
                exit_code, output, _ = self.ssh_connection.execute_command(
                    f"ls -lh {remote_bundle_path}"
                )
                if exit_code == 0:
                    console.print(f"[green]✓ Bundle found: {output.strip()}[/green]")
                return True
            else:
                console.print(f"[red]✗ Bundle file not found: {remote_bundle_path}[/red]")
                return False
                
        except Exception as e:
            console.print(f"[red]✗ Error verifying bundle: {e}[/red]")
            return False
    
    def _check_rauc_status(self) -> bool:
        """Check RAUC system status before installation."""
        try:
            exit_code, output, stderr = self.ssh_connection.execute_command("rauc status")
            
            if exit_code == 0:
                console.print("[green]✓ RAUC status check passed[/green]")
                # Parse and display key information
                self._display_rauc_status(output)
                return True
            else:
                console.print(f"[red]✗ RAUC status check failed: {stderr.strip()}[/red]")
                return False
                
        except Exception as e:
            console.print(f"[red]✗ Error checking RAUC status: {e}[/red]")
            return False
    
    def _display_rauc_status(self, status_output: str):
        """Display formatted RAUC status information."""
        try:
            lines = status_output.strip().split('\\n')
            
            table = Table(title="RAUC System Status", show_header=True, header_style="bold blue")
            table.add_column("Property", style="cyan")
            table.add_column("Value", style="white")
            
            for line in lines:
                if '=' in line:
                    key, value = line.split('=', 1)
                    key = key.strip()
                    value = value.strip()
                    
                    # Highlight important values
                    if 'booted' in key.lower():
                        value = f"[green]{value}[/green]"
                    elif 'compatible' in key.lower():
                        value = f"[blue]{value}[/blue]"
                    
                    table.add_row(key, value)
            
            console.print(table)
            
        except Exception:
            # Fallback to simple display
            console.print(f"[blue]Status: {status_output.strip()}[/blue]")
    
    def _build_install_command(self, remote_bundle_path: str) -> str:
        """Build RAUC install command with options."""
        cmd_parts = ["rauc", "install"]
        
        if self.config.ignore_compatible:
            cmd_parts.append("--ignore-compatible")
        
        cmd_parts.append(remote_bundle_path)
        
        return " ".join(cmd_parts)
    
    def _execute_with_progress(
        self,
        command: str,
        progress_callback: Optional[Callable[[InstallProgress], None]] = None
    ) -> bool:
        """Execute RAUC install command with progress monitoring."""
        self._install_running = True
        
        try:
            # Start the installation command
            stdin, stdout, stderr = self.ssh_connection.client.exec_command(
                command, timeout=self.config.timeout
            )
            
            # Monitor progress
            with Progress(
                SpinnerColumn(),
                TextColumn("[bold blue]{task.description}"),
                BarColumn(bar_width=None),
                "[progress.percentage]{task.percentage:>3.1f}%",
                console=console
            ) as progress:
                
                task_id = progress.add_task("Installing RAUC bundle", total=100)
                
                progress_info = InstallProgress()
                last_percentage = 0
                
                # Monitor command output
                while self._install_running:
                    # Check if command is still running
                    if stdout.channel.exit_status_ready():
                        break
                    
                    # Read available output
                    if stdout.channel.recv_ready():
                        output = stdout.channel.recv(1024).decode('utf-8', errors='ignore')
                        
                        # Parse progress from output
                        new_progress = self._parse_progress_output(output, progress_info)
                        
                        if new_progress.percentage > last_percentage:
                            progress.update(
                                task_id,
                                completed=new_progress.percentage,
                                description=f"Installing: {new_progress.message or 'Processing...'}"
                            )
                            last_percentage = new_progress.percentage
                        
                        if progress_callback:
                            progress_callback(new_progress)
                    
                    time.sleep(self.config.progress_interval)
                
                # Get final result
                exit_code = stdout.channel.recv_exit_status()
                final_stdout = stdout.read().decode('utf-8')
                final_stderr = stderr.read().decode('utf-8')
                
                # Update progress to 100% if successful
                if exit_code == 0:
                    progress.update(task_id, completed=100, description="Installation completed")
                    progress_info.percentage = 100
                    progress_info.completed = True
                    progress_info.message = "Installation successful"
                else:
                    progress_info.error = final_stderr.strip() or "Installation failed"
                
                if progress_callback:
                    progress_callback(progress_info)
            
            # Handle results
            if exit_code == 0:
                console.print("[green]✓ RAUC installation completed successfully[/green]")
                self._display_install_result(final_stdout)
                return True
            else:
                console.print(f"[red]✗ RAUC installation failed (exit code: {exit_code})[/red]")
                if final_stderr:
                    console.print(f"[red]Error: {final_stderr.strip()}[/red]")
                return False
                
        except Exception as e:
            console.print(f"[red]✗ Installation error: {e}[/red]")
            return False
            
        finally:
            self._install_running = False
    
    def _parse_progress_output(self, output: str, current_progress: InstallProgress) -> InstallProgress:
        """Parse RAUC progress output."""
        try:
            # Look for percentage patterns
            percentage_patterns = [
                r'(\d+)%',  # Simple percentage
                r'Progress: (\d+)%',  # Progress: XX%
                r'Installing.*?(\d+)%',  # Installing ... XX%
            ]
            
            for pattern in percentage_patterns:
                match = re.search(pattern, output)
                if match:
                    current_progress.percentage = int(match.group(1))
                    break
            
            # Look for operation descriptions
            operation_patterns = [
                r'Installing (.*?)\\n',
                r'Copying (.*?)\\n',
                r'Writing (.*?)\\n',
                r'Mounting (.*?)\\n',
            ]
            
            for pattern in operation_patterns:
                match = re.search(pattern, output)
                if match:
                    current_progress.operation = match.group(1).strip()
                    current_progress.message = current_progress.operation
                    break
            
            # Look for slot information
            slot_match = re.search(r'slot\\.(\\w+)', output)
            if slot_match:
                current_progress.slot = slot_match.group(1)
            
            # Look for image information
            image_match = re.search(r'(\\w+\\.\\w+)\\s+to\\s+/dev', output)
            if image_match:
                current_progress.image = image_match.group(1)
            
            return current_progress
            
        except Exception:
            return current_progress
    
    def _display_install_result(self, output: str):
        """Display installation result information."""
        try:
            if output.strip():
                console.print(Panel(
                    output.strip(),
                    title="Installation Output",
                    title_align="left",
                    border_style="green"
                ))
        except Exception:
            pass
    
    def get_install_status(self) -> Dict[str, str]:
        """Get current installation status from RAUC."""
        try:
            exit_code, output, _ = self.ssh_connection.execute_command("rauc status")
            
            if exit_code == 0:
                status = {}
                for line in output.strip().split('\\n'):
                    if '=' in line:
                        key, value = line.split('=', 1)
                        status[key.strip()] = value.strip()
                return status
            
            return {}
            
        except Exception:
            return {}
    
    def mark_good(self) -> bool:
        """Mark current slot as good (prevent rollback)."""
        try:
            console.print("[blue]Marking current slot as good...[/blue]")
            
            exit_code, output, stderr = self.ssh_connection.execute_command("rauc status mark-good")
            
            if exit_code == 0:
                console.print("[green]✓ Slot marked as good[/green]")
                return True
            else:
                console.print(f"[red]✗ Failed to mark slot as good: {stderr.strip()}[/red]")
                return False
                
        except Exception as e:
            console.print(f"[red]✗ Error marking slot as good: {e}[/red]")
            return False


def install_rauc_bundle(
    ssh_connection: SSHConnection,
    remote_bundle_path: str,
    config: Optional[InstallConfig] = None,
    progress_callback: Optional[Callable[[InstallProgress], None]] = None
) -> bool:
    """Install RAUC bundle on target device.
    
    Args:
        ssh_connection: Active SSH connection
        remote_bundle_path: Path to bundle on remote device
        config: Installation configuration
        progress_callback: Optional progress callback
        
    Returns:
        True if installation successful, False otherwise
    """
    installer = RaucInstaller(ssh_connection, config)
    return installer.install_bundle(remote_bundle_path, progress_callback)