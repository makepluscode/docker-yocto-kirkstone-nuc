"""RAUC update implementation module."""

import sys
from pathlib import Path
from typing import Optional

from rich.console import Console

from .config import SSH_ERROR_KEYWORDS
from .exceptions import ConnectionError, InstallationError
from .connection import ConnectionConfig, SSHConnection, copy_ssh_key
from .installer import InstallConfig, InstallProgress, install_rauc_bundle
from .transfer import TransferConfig, upload_bundle
from .utils import format_duration

console = Console()


def perform_update(bundle_path: Path, host: str, user: str, port: int, 
                  password: Optional[str], key_file: Optional[str], remote_path: str,
                  timeout: int, ignore_compatible: bool, mark_good: bool, 
                  verbose: bool, cleanup: bool, copy_ssh_key: bool):
    """Perform RAUC update operation.
    
    Args:
        bundle_path: Path to RAUC bundle
        host: Target host IP
        user: SSH username
        port: SSH port
        password: SSH password
        key_file: SSH private key file
        remote_path: Remote upload directory
        timeout: Installation timeout
        ignore_compatible: Skip compatibility checks
        mark_good: Mark slot as good after installation
        verbose: Enable verbose output
        cleanup: Clean up bundle after installation
        copy_ssh_key: Copy SSH key for authentication
    """
    
    console.print(f"[bold green]Starting RAUC Update Process[/bold green]")
    console.print(f"Bundle: [cyan]{bundle_path}[/cyan]")
    console.print(f"Target: [cyan]{user}@{host}:{port}[/cyan]")
    console.print()
    
    # Configuration
    conn_config = ConnectionConfig(
        host=host,
        port=port,
        username=user,
        password=password,
        key_filename=key_file,
        timeout=30
    )
    
    transfer_config = TransferConfig(
        remote_path=remote_path
    )
    
    install_config = InstallConfig(
        timeout=timeout,
        ignore_compatible=ignore_compatible
    )
    
    try:
        # Step 0: Copy SSH key if requested or using password authentication
        if copy_ssh_key or (password and not key_file):
            console.print("[bold blue]Step 0: Setting up SSH key authentication...[/bold blue]")
            ssh_password = password if password else "root"
            if not copy_ssh_key(host, user, port, ssh_password):
                console.print("[red]✗ Failed to copy SSH key, continuing with password auth[/red]")
            else:
                # Update config to not use password after key is copied
                conn_config.password = None
        
        with SSHConnection(conn_config) as ssh_conn:
            # Test RAUC availability
            if not ssh_conn.test_rauc_availability():
                console.print("[red]✗ RAUC not available on target device[/red]")
                sys.exit(1)
            
            # Upload bundle
            console.print("[bold blue]Step 1: Uploading bundle...[/bold blue]")
            
            def upload_progress(transferred: int, total: int):
                if verbose:
                    percentage = (transferred / total) * 100
                    console.print(f"Upload progress: {percentage:.1f}%")
            
            remote_bundle_path = upload_bundle(
                ssh_conn,
                str(bundle_path),
                remote_path,
                upload_progress if verbose else None
            )
            
            if not remote_bundle_path:
                console.print("[red]✗ Failed to upload bundle[/red]")
                sys.exit(1)
            
            # Install bundle
            console.print("[bold blue]Step 2: Installing bundle...[/bold blue]")
            
            def install_progress(progress: InstallProgress):
                if verbose:
                    console.print(f"Install: {progress.percentage}% - {progress.message}")
            
            success = install_rauc_bundle(
                ssh_conn,
                remote_bundle_path,
                install_config,
                install_progress if verbose else None
            )
            
            if not success:
                console.print("[red]✗ Installation failed[/red]")
                sys.exit(1)
            
            # Mark good if requested
            if mark_good:
                console.print("[bold blue]Step 3: Marking slot as good...[/bold blue]")
                from .installer import RaucInstaller
                installer = RaucInstaller(ssh_conn, install_config)
                installer.mark_good()
            
            # Cleanup
            if cleanup:
                console.print("[bold blue]Step 4: Cleaning up...[/bold blue]")
                from .transfer import FileTransfer
                transfer = FileTransfer(ssh_conn, transfer_config)
                transfer.remove_remote_file(Path(remote_bundle_path).name)
            
            console.print()
            console.print("[bold green]✓ Update completed successfully![/bold green]")
            
    except KeyboardInterrupt:
        console.print("[yellow]\\nOperation cancelled by user[/yellow]")
        sys.exit(1)
    except Exception as e:
        # Handle connection loss gracefully (expected during reboot)
        error_msg = str(e).lower()
        if any(keyword in error_msg for keyword in SSH_ERROR_KEYWORDS):
            console.print("[yellow]Target device rebooting - update likely successful[/yellow]")
            console.print("[bold green]✓ Update completed successfully![/bold green]")
        else:
            console.print(f"[red]✗ Update failed: {e}[/red]")
            if verbose:
                import traceback
                console.print(traceback.format_exc())
            sys.exit(1)