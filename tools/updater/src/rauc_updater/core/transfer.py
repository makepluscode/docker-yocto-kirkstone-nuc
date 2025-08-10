"""File transfer functionality with progress tracking."""

import os
import time
from pathlib import Path
from typing import Callable, Optional

import paramiko
from pydantic import BaseModel
from rich.console import Console
from rich.progress import (
    BarColumn,
    DownloadColumn,
    Progress,
    TaskID,
    TextColumn,
    TimeRemainingColumn,
    TransferSpeedColumn,
)

from rauc_updater.core.connection import SSHConnection

console = Console()


class TransferConfig(BaseModel):
    """Configuration for file transfer."""
    
    remote_path: str = "/tmp"
    chunk_size: int = 32768  # 32KB chunks
    verify_checksum: bool = False
    overwrite: bool = True
    
    class Config:
        arbitrary_types_allowed = True


class FileTransfer:
    """Handles SCP file transfer with progress tracking."""
    
    def __init__(self, ssh_connection: SSHConnection, config: Optional[TransferConfig] = None):
        self.ssh_connection = ssh_connection
        self.config = config or TransferConfig()
        self.scp_client: Optional[paramiko.SFTPClient] = None
    
    def _setup_sftp(self) -> bool:
        """Setup SFTP client."""
        try:
            if not self.ssh_connection.is_connected():
                console.print("[red]✗ SSH connection not established[/red]")
                return False
            
            self.scp_client = self.ssh_connection.client.open_sftp()
            return True
            
        except Exception as e:
            console.print(f"[red]✗ Failed to setup SFTP: {e}[/red]")
            return False
    
    def _cleanup_sftp(self):
        """Cleanup SFTP client."""
        if self.scp_client:
            self.scp_client.close()
            self.scp_client = None
    
    def upload_file(
        self,
        local_path: str,
        remote_filename: Optional[str] = None,
        progress_callback: Optional[Callable[[int, int], None]] = None
    ) -> bool:
        """Upload file to remote host with progress tracking.
        
        Args:
            local_path: Path to local file
            remote_filename: Remote filename (defaults to local filename)
            progress_callback: Optional callback for progress updates
            
        Returns:
            True if upload successful, False otherwise
        """
        local_file = Path(local_path)
        
        if not local_file.exists():
            console.print(f"[red]✗ Local file not found: {local_path}[/red]")
            return False
        
        if not local_file.is_file():
            console.print(f"[red]✗ Path is not a file: {local_path}[/red]")
            return False
        
        # Determine remote path
        if remote_filename is None:
            remote_filename = local_file.name
        
        remote_path = f"{self.config.remote_path}/{remote_filename}"
        
        console.print(f"[blue]Uploading {local_file.name} to {remote_path}...[/blue]")
        
        # Setup SFTP
        if not self._setup_sftp():
            return False
        
        try:
            file_size = local_file.stat().st_size
            
            # Check remote disk space
            if not self._check_remote_space(file_size):
                return False
            
            # Create progress bar
            with Progress(
                TextColumn("[bold blue]{task.description}"),
                BarColumn(bar_width=None),
                "[progress.percentage]{task.percentage:>3.1f}%",
                "•",
                DownloadColumn(),
                "•",
                TransferSpeedColumn(),
                "•",
                TimeRemainingColumn(),
                console=console
            ) as progress:
                
                task_id = progress.add_task(
                    f"Uploading {local_file.name}",
                    total=file_size
                )
                
                # Custom progress callback
                def _progress_callback(transferred: int, total: int):
                    progress.update(task_id, completed=transferred)
                    if progress_callback:
                        progress_callback(transferred, total)
                
                # Perform the upload
                self.scp_client.put(
                    str(local_file),
                    remote_path,
                    callback=_progress_callback,
                    confirm=True
                )
            
            console.print(f"[green]✓ Upload completed: {remote_path}[/green]")
            
            # Verify file size
            if self._verify_upload(local_file, remote_path):
                return True
            else:
                console.print("[red]✗ Upload verification failed[/red]")
                return False
                
        except Exception as e:
            console.print(f"[red]✗ Upload failed: {e}[/red]")
            return False
            
        finally:
            self._cleanup_sftp()
    
    def _check_remote_space(self, required_bytes: int) -> bool:
        """Check if remote has enough space for the file."""
        try:
            disk_info = self.ssh_connection.get_disk_space(self.config.remote_path)
            if disk_info:
                # Parse available space (e.g., "1.2G" -> bytes)
                available_str = disk_info['available']
                
                # Simple parsing for common units
                available_bytes = self._parse_size_string(available_str)
                
                if available_bytes and available_bytes < required_bytes * 1.1:  # 10% buffer
                    console.print(
                        f"[red]✗ Insufficient disk space. "
                        f"Required: {self._format_bytes(required_bytes)}, "
                        f"Available: {available_str}[/red]"
                    )
                    return False
                
                console.print(
                    f"[blue]Disk space check: {available_str} available[/blue]"
                )
            
            return True
            
        except Exception as e:
            console.print(f"[yellow]Warning: Could not check disk space: {e}[/yellow]")
            return True  # Proceed anyway
    
    def _parse_size_string(self, size_str: str) -> Optional[int]:
        """Parse size string like '1.2G' to bytes."""
        try:
            size_str = size_str.strip().upper()
            
            multipliers = {
                'K': 1024,
                'M': 1024 ** 2,
                'G': 1024 ** 3,
                'T': 1024 ** 4
            }
            
            if size_str[-1] in multipliers:
                number = float(size_str[:-1])
                return int(number * multipliers[size_str[-1]])
            else:
                return int(size_str)
                
        except (ValueError, IndexError):
            return None
    
    def _format_bytes(self, bytes_count: int) -> str:
        """Format bytes to human readable string."""
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if bytes_count < 1024.0:
                return f"{bytes_count:.1f}{unit}"
            bytes_count /= 1024.0
        return f"{bytes_count:.1f}PB"
    
    def _verify_upload(self, local_file: Path, remote_path: str) -> bool:
        """Verify uploaded file matches local file."""
        try:
            local_size = local_file.stat().st_size
            remote_stat = self.scp_client.stat(remote_path)
            remote_size = remote_stat.st_size
            
            if local_size != remote_size:
                console.print(
                    f"[red]Size mismatch: local={local_size}, remote={remote_size}[/red]"
                )
                return False
            
            console.print(f"[green]✓ File size verified: {self._format_bytes(local_size)}[/green]")
            return True
            
        except Exception as e:
            console.print(f"[yellow]Warning: Could not verify upload: {e}[/yellow]")
            return True  # Assume success if we can't verify
    
    def remove_remote_file(self, remote_filename: str) -> bool:
        """Remove file from remote host."""
        remote_path = f"{self.config.remote_path}/{remote_filename}"
        
        try:
            exit_code, _, stderr = self.ssh_connection.execute_command(f"rm -f {remote_path}")
            if exit_code == 0:
                console.print(f"[green]✓ Removed remote file: {remote_path}[/green]")
                return True
            else:
                console.print(f"[red]✗ Failed to remove remote file: {stderr.strip()}[/red]")
                return False
                
        except Exception as e:
            console.print(f"[red]✗ Error removing remote file: {e}[/red]")
            return False
    
    def list_remote_files(self, pattern: str = "*.raucb") -> list:
        """List files in remote directory matching pattern."""
        try:
            command = f"ls -la {self.config.remote_path}/{pattern} 2>/dev/null || true"
            exit_code, stdout, _ = self.ssh_connection.execute_command(command)
            
            files = []
            if stdout.strip():
                for line in stdout.strip().split('\n'):
                    if line and not line.startswith('total'):
                        fields = line.split()
                        if len(fields) >= 9:
                            filename = fields[-1]
                            size = fields[4]
                            files.append({"name": filename, "size": size})
            
            return files
            
        except Exception as e:
            console.print(f"[yellow]Warning: Could not list remote files: {e}[/yellow]")
            return []


def upload_bundle(
    ssh_connection: SSHConnection,
    bundle_path: str,
    remote_path: str = "/tmp",
    progress_callback: Optional[Callable[[int, int], None]] = None
) -> Optional[str]:
    """Upload RAUC bundle to target device.
    
    Returns:
        Remote file path if successful, None otherwise
    """
    config = TransferConfig(remote_path=remote_path)
    transfer = FileTransfer(ssh_connection, config)
    
    bundle_file = Path(bundle_path)
    if not bundle_file.suffix == '.raucb':
        console.print("[yellow]Warning: File doesn't have .raucb extension[/yellow]")
    
    if transfer.upload_file(bundle_path, progress_callback=progress_callback):
        return f"{remote_path}/{bundle_file.name}"
    return None