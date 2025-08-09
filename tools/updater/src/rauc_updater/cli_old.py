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
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
def dbus_status(
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str]
):
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
                "dbus-send --system --print-reply --dest=de.pengutronix.rauc "
                "/de/pengutronix/rauc/Installer de.pengutronix.rauc.Installer.GetSlotStatus"
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
                exit_code, output, stderr = ssh_conn.execute_command("rauc status")
                if exit_code == 0:
                    console.print(Panel(
                        output.strip(),
                        title="RAUC System Status (Fallback)",
                        border_style="yellow"
                    ))
                
    except Exception as e:
        console.print(f"[red]✗ Failed to get D-Bus status: {e}[/red]")
        sys.exit(1)


@cli.command()
@click.argument('bundle_path', type=click.Path(exists=True, path_type=Path))
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
@click.option('--use-dbus', is_flag=True, help='Use D-Bus interface for installation')
@click.option('--remote-path', default='/tmp', help='Remote directory for bundle upload')
@click.option('--timeout', default=600, help='Installation timeout in seconds')
@click.option('--verbose', '-v', is_flag=True, help='Verbose output')
@click.option('--cleanup', is_flag=True, default=True, help='Remove bundle after installation')
def dbus_update(
    bundle_path: Path,
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str],
    use_dbus: bool,
    remote_path: str,
    timeout: int,
    verbose: bool,
    cleanup: bool
):
    """Update target device using D-Bus interface."""
    
    console.print(f"[bold green]Starting D-Bus RAUC Update Process[/bold green]")
    console.print(f"Bundle: [cyan]{bundle_path}[/cyan]")
    console.print(f"Target: [cyan]{user}@{host}:{port}[/cyan]")
    console.print(f"Method: [cyan]{'D-Bus' if use_dbus else 'SSH+D-Bus'}[/cyan]")
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
    
    try:
        with SSHConnection(conn_config) as ssh_conn:
            # Test D-Bus availability
            exit_code, output, stderr = ssh_conn.execute_command(
                "dbus-send --system --print-reply --dest=de.pengutronix.rauc "
                "/de/pengutronix/rauc/Installer org.freedesktop.DBus.Introspectable.Introspect > /dev/null 2>&1"
            )
            
            if exit_code != 0:
                console.print("[red]✗ RAUC D-Bus interface not available[/red]")
                console.print("[yellow]Falling back to standard update method...[/yellow]")
                # Fallback to regular update
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
            
            # Install via D-Bus
            console.print("[bold blue]Step 2: Installing via D-Bus...[/bold blue]")
            
            dbus_cmd = (
                f"dbus-send --system --print-reply --dest=de.pengutronix.rauc "
                f"/de/pengutronix/rauc/Installer de.pengutronix.rauc.Installer.Install "
                f"string:\"{remote_bundle_path}\""
            )
            
            if verbose:
                console.print(f"D-Bus command: {dbus_cmd}")
            
            exit_code, output, stderr = ssh_conn.execute_command(dbus_cmd)
            
            if exit_code == 0:
                console.print("[bold green]✓ D-Bus installation initiated successfully![/bold green]")
                if verbose:
                    console.print(Panel(output.strip(), title="D-Bus Response", border_style="green"))
            else:
                console.print(f"[red]✗ D-Bus installation failed: {stderr.strip()}[/red]")
                sys.exit(1)
            
            # Cleanup
            if cleanup:
                console.print("[bold blue]Step 3: Cleaning up...[/bold blue]")
                from .core.transfer import FileTransfer
                transfer = FileTransfer(ssh_conn, transfer_config)
                transfer.remove_remote_file(Path(remote_bundle_path).name)
            
            console.print()
            console.print("[bold green]✓ D-Bus update completed successfully![/bold green]")
            
    except KeyboardInterrupt:
        console.print("[yellow]\\nOperation cancelled by user[/yellow]")
        sys.exit(1)
    except Exception as e:
        console.print(f"[red]✗ D-Bus update failed: {e}[/red]")
        if verbose:
            import traceback
            console.print(traceback.format_exc())
        sys.exit(1)


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


@cli.command()
@click.option('--interface', '-i', default='enp42s0', help='Network interface to configure')
@click.option('--host-ip', default='192.168.1.101', help='Host IP address to set')
@click.option('--netmask', default='255.255.255.0', help='Network mask')
@click.option('--target-ip', default='192.168.1.100', help='Target device IP address')
@click.option('--target-user', default='root', help='Target device username')
@click.option('--skip-network', is_flag=True, help='Skip network interface configuration')
def connect(
    interface: str,
    host_ip: str,
    netmask: str,
    target_ip: str,
    target_user: str,
    skip_network: bool
):
    """Setup network connection and SSH authentication to target device."""
    import subprocess
    import os
    from pathlib import Path
    
    console.print("[bold blue]Setting up connection to target device...[/bold blue]")
    console.print(f"Interface: [cyan]{interface}[/cyan]")
    console.print(f"Host IP: [cyan]{host_ip}[/cyan]")
    console.print(f"Target: [cyan]{target_user}@{target_ip}[/cyan]")
    console.print()
    
    try:
        # Step 1: Network configuration
        if not skip_network:
            console.print("[bold blue]Step 1: Configuring network interface...[/bold blue]")
            
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
                    console.print(f"[red]Failed to configure network interface: {result.stderr}[/red]")
                    sys.exit(1)
                console.print(f"[green]✓ Network interface configured[/green]")
            else:
                console.print(f"[green]✓ Network interface already configured[/green]")
        
        # Step 2: SSH setup
        console.print("[bold blue]Step 2: Setting up SSH authentication...[/bold blue]")
        
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
        else:
            console.print("[yellow]Setting up SSH key authentication...[/yellow]")
            console.print("You may need to enter the target device password once.")
            
            # Try ssh-copy-id
            result = subprocess.run(['ssh-copy-id', '-o', 'ConnectTimeout=10',
                                   f'{target_user}@{target_ip}'])
            
            if result.returncode == 0:
                console.print("[bold green]✓ SSH key setup successful![/bold green]")
            else:
                console.print("[red]✗ SSH key setup failed[/red]")
                console.print("Manual setup may be required.")
                sys.exit(1)
        
        # Final connection test
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


@cli.command()
@click.option('--host', '-h', default='192.168.1.100', help='Target host IP address')
@click.option('--user', '-u', default='root', help='SSH username')
@click.option('--port', '-p', default=22, help='SSH port')
@click.option('--password', help='SSH password (not recommended)')
@click.option('--key-file', '-k', type=click.Path(exists=True), help='SSH private key file')
@click.option('--action', type=click.Choice(['status', 'start', 'stop', 'restart', 'config']), 
              default='status', help='Hawkbit client action')
@click.option('--config-server', help='Hawkbit server URL for config action')
@click.option('--config-tenant', default='DEFAULT', help='Hawkbit tenant name')
@click.option('--config-controller-id', help='Controller ID for Hawkbit')
def hawkbit(
    host: str,
    user: str,
    port: int,
    password: Optional[str],
    key_file: Optional[str],
    action: str,
    config_server: Optional[str],
    config_tenant: str,
    config_controller_id: Optional[str]
):
    """Control Hawkbit client on target device."""
    
    config = ConnectionConfig(
        host=host,
        port=port,
        username=user,
        password=password,
        key_filename=key_file
    )
    
    try:
        with SSHConnection(config) as ssh_conn:
            console.print(f"[bold blue]Hawkbit Client Control - {action.upper()}[/bold blue]")
            
            if action == 'status':
                # Check if hawkbit service is running
                exit_code, output, stderr = ssh_conn.execute_command(
                    "systemctl is-active rauc-hawkbit.service"
                )
                
                if exit_code == 0:
                    console.print("[green]✓ Hawkbit client service is running[/green]")
                    
                    # Get more detailed status
                    exit_code, output, stderr = ssh_conn.execute_command(
                        "systemctl status rauc-hawkbit.service"
                    )
                    
                    if exit_code == 0:
                        console.print(Panel(
                            output.strip(),
                            title="Hawkbit Service Status",
                            border_style="green"
                        ))
                else:
                    console.print("[red]✗ Hawkbit client service is not running[/red]")
                    
                # Check config file
                exit_code, output, stderr = ssh_conn.execute_command(
                    "cat /etc/rauc-hawkbit/config.cfg"
                )
                
                if exit_code == 0:
                    console.print(Panel(
                        output.strip(),
                        title="Hawkbit Configuration",
                        border_style="blue"
                    ))
                    
            elif action in ['start', 'stop', 'restart']:
                console.print(f"[yellow]{action.capitalize()}ing Hawkbit client service...[/yellow]")
                
                exit_code, output, stderr = ssh_conn.execute_command(
                    f"systemctl {action} rauc-hawkbit.service"
                )
                
                if exit_code == 0:
                    console.print(f"[green]✓ Hawkbit service {action}ed successfully[/green]")
                else:
                    console.print(f"[red]✗ Failed to {action} Hawkbit service: {stderr.strip()}[/red]")
                    
            elif action == 'config':
                if not config_server:
                    console.print("[red]✗ --config-server is required for config action[/red]")
                    sys.exit(1)
                    
                if not config_controller_id:
                    config_controller_id = f"nuc-{host.replace('.', '-')}"
                    
                console.print(f"[yellow]Configuring Hawkbit client...[/yellow]")
                console.print(f"Server: [cyan]{config_server}[/cyan]")
                console.print(f"Tenant: [cyan]{config_tenant}[/cyan]")
                console.print(f"Controller ID: [cyan]{config_controller_id}[/cyan]")
                
                # Create hawkbit config
                hawkbit_config = f"""[client]
hawkbit_server = {config_server}
ssl = true
tenant_id = {config_tenant}
target_name = {config_controller_id}
auth_token = 
bundle_dl_location = /tmp/hawkbit_bundle.raucb

[device]
product = Intel-NUC
model = NUC-Series
serialnumber = {config_controller_id}
version = 1.0
"""
                
                # Write config file
                exit_code, output, stderr = ssh_conn.execute_command(
                    f"echo '{hawkbit_config}' | sudo tee /etc/rauc-hawkbit/config.cfg"
                )
                
                if exit_code == 0:
                    console.print("[green]✓ Hawkbit configuration updated[/green]")
                    
                    # Restart service to apply new config
                    exit_code, output, stderr = ssh_conn.execute_command(
                        "systemctl restart rauc-hawkbit.service"
                    )
                    
                    if exit_code == 0:
                        console.print("[green]✓ Hawkbit service restarted with new configuration[/green]")
                    else:
                        console.print(f"[yellow]⚠ Configuration saved but service restart failed: {stderr.strip()}[/yellow]")
                else:
                    console.print(f"[red]✗ Failed to update configuration: {stderr.strip()}[/red]")
                    sys.exit(1)
                    
    except Exception as e:
        console.print(f"[red]✗ Hawkbit operation failed: {e}[/red]")
        sys.exit(1)


def main():
    """Main CLI entry point."""
    cli()


if __name__ == '__main__':
    main()