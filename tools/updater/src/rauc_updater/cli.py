"""Command line interface for RAUC updater."""

import sys
from pathlib import Path
from typing import Optional

import click
from rich.console import Console
from rich.panel import Panel

from rauc_updater.core.connection import ConnectionConfig, SSHConnection, test_connection, copy_ssh_key
from rauc_updater.core.installer import InstallConfig, InstallProgress, install_rauc_bundle
from rauc_updater.core.transfer import TransferConfig, upload_bundle

console = Console()


@click.group()
@click.version_option()
def cli():
    """RAUC Updater Tool - Deploy RAUC bundles to target devices."""
    console.print(Panel.fit(
        "[bold blue]RAUC Updater Tool[/bold blue]\\n"
        "Step 1: Headless CLI Version\\n"
        "Deploy RAUC bundles over SSH/SCP",
        border_style="blue"
    ))


@cli.command()
@click.argument('bundle_path', type=click.Path(exists=True, path_type=Path))
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
@click.option('--remote-path', default='/tmp', help='Remote directory for bundle upload')
@click.option('--timeout', default=600, help='Installation timeout in seconds')
@click.option('--ignore-compatible', is_flag=True, help='Ignore compatibility checks')
@click.option('--mark-good', is_flag=True, help='Mark slot as good after installation')
@click.option('--verbose', '-v', is_flag=True, help='Verbose output')
@click.option('--cleanup', is_flag=True, default=True, help='Remove bundle after installation')
@click.option('--copy-ssh-key', is_flag=True, help='Copy SSH key using default password "root"')
def update(
    bundle_path: Path,
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str],
    remote_path: str,
    timeout: int,
    ignore_compatible: bool,
    mark_good: bool,
    verbose: bool,
    cleanup: bool,
    copy_ssh_key: bool
):
    """Update target device with RAUC bundle."""
    
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
                from .core.installer import RaucInstaller
                installer = RaucInstaller(ssh_conn, install_config)
                installer.mark_good()
            
            # Cleanup
            if cleanup:
                console.print("[bold blue]Step 4: Cleaning up...[/bold blue]")
                from .core.transfer import FileTransfer
                transfer = FileTransfer(ssh_conn, transfer_config)
                transfer.remove_remote_file(Path(remote_bundle_path).name)
            
            console.print()
            console.print("[bold green]✓ Update completed successfully![/bold green]")
            
    except KeyboardInterrupt:
        console.print("[yellow]\\nOperation cancelled by user[/yellow]")
        sys.exit(1)
    except Exception as e:
        console.print(f"[red]✗ Update failed: {e}[/red]")
        if verbose:
            import traceback
            console.print(traceback.format_exc())
        sys.exit(1)


@cli.command()
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
def test(
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str]
):
    """Test connection to target device."""
    
    console.print(f"[bold blue]Testing connection to {user}@{host}:{port}[/bold blue]")
    
    # Create configuration
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


@cli.command()
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
def status(
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str]
):
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


@cli.command()
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', default='root', help='SSH password for key copying')
def setup_ssh(
    host: str,
    user: str,
    port: int,
    password: str
):
    """Copy SSH public key to target device for passwordless authentication."""
    
    console.print(f"[bold blue]Setting up SSH key authentication for {user}@{host}:{port}[/bold blue]")
    
    try:
        if copy_ssh_key(host, user, port, password):
            console.print("[bold green]✓ SSH key setup completed successfully![/bold green]")
            console.print("You can now connect without a password.")
        else:
            console.print("[red]✗ SSH key setup failed[/red]")
            sys.exit(1)
            
    except Exception as e:
        console.print(f"[red]✗ SSH key setup error: {e}[/red]")
        sys.exit(1)


@cli.command()
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
def cleanup(
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str]
):
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
            from .core.transfer import FileTransfer, TransferConfig
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


def main():
    """Main CLI entry point."""
    cli()


if __name__ == '__main__':
    main()