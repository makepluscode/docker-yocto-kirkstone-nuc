# RAUC Updater Usage Guide

## Overview

The RAUC Updater Tool (Step 1 - Headless CLI Version) provides a command-line interface for deploying RAUC bundles to target devices over SSH/SCP.

## Installation

```bash
cd rauc-updater
uv sync
```

## Basic Usage

### Test Connection

Before deploying bundles, test the connection to your target device:

```bash
# Test with default configuration (192.168.1.100)
uv run rauc-updater test

# Test with custom host
uv run rauc-updater test --host 192.168.1.150

# Test with custom user and port
uv run rauc-updater test --host 192.168.1.100 --user admin --port 2222

# Test with SSH key file
uv run rauc-updater test --key-file ~/.ssh/my_key
```

### Deploy RAUC Bundle

Deploy a RAUC bundle to your target device:

```bash
# Basic deployment
uv run rauc-updater update /path/to/bundle.raucb

# Deploy to custom target
uv run rauc-updater update /path/to/bundle.raucb --host 192.168.1.150

# Deploy with verbose output
uv run rauc-updater update /path/to/bundle.raucb --verbose

# Deploy and mark slot as good
uv run rauc-updater update /path/to/bundle.raucb --mark-good

# Deploy with custom options
uv run rauc-updater update /path/to/bundle.raucb \\
    --host 192.168.1.100 \\
    --user root \\
    --remote-path /data \\
    --timeout 900 \\
    --ignore-compatible \\
    --verbose
```

### Check RAUC Status

Check the current RAUC status on the target device:

```bash
# Check status with default configuration
uv run rauc-updater status

# Check status on custom host
uv run rauc-updater status --host 192.168.1.150
```

### Clean Up Files

Remove temporary bundle files from the target device:

```bash
# Clean up with default configuration
uv run rauc-updater cleanup

# Clean up on custom host
uv run rauc-updater cleanup --host 192.168.1.150
```

## Configuration Options

### Connection Settings

| Option | Default | Description |
|--------|---------|-------------|
| `--host` | 192.168.1.100 | Target device IP address |
| `--user` | root | SSH username |
| `--port` | 22 | SSH port |
| `--password` | None | SSH password (not recommended) |
| `--key-file` | Auto-detect | SSH private key file |

### Transfer Settings

| Option | Default | Description |
|--------|---------|-------------|
| `--remote-path` | /tmp | Remote directory for bundle upload |
| `--cleanup` | True | Remove bundle after installation |

### Installation Settings

| Option | Default | Description |
|--------|---------|-------------|
| `--timeout` | 600 | Installation timeout in seconds |
| `--ignore-compatible` | False | Ignore compatibility checks |
| `--mark-good` | False | Mark slot as good after installation |

## Authentication

The tool supports multiple authentication methods:

### 1. SSH Key Authentication (Recommended)

```bash
# Use specific key file
uv run rauc-updater test --key-file ~/.ssh/target_device_key

# Auto-detect common key files (id_rsa, id_ecdsa, id_ed25519)
uv run rauc-updater test
```

### 2. Password Authentication

```bash
# Interactive password prompt
uv run rauc-updater test --password

# Password from command line (not recommended)
uv run rauc-updater test --password "your_password"
```

## Examples

### Example 1: Basic Update

```bash
# 1. Test connection
uv run rauc-updater test

# 2. Deploy bundle
uv run rauc-updater update /data/nuc-image-qt5-bundle-intel-corei7-64.raucb

# 3. Check status
uv run rauc-updater status
```

### Example 2: Update with Verification

```bash
# Deploy bundle and mark as good
uv run rauc-updater update /data/bundle.raucb --mark-good --verbose
```

### Example 3: Custom Target

```bash
# Update device with custom IP and settings
uv run rauc-updater update /data/bundle.raucb \\
    --host 192.168.1.150 \\
    --user admin \\
    --key-file ~/.ssh/factory_key \\
    --remote-path /data \\
    --timeout 1200 \\
    --verbose
```

### Example 4: Batch Operations

```bash
#!/bin/bash
# batch_update.sh - Update multiple devices

BUNDLE="/data/firmware-v2.1.0.raucb"
DEVICES=("192.168.1.100" "192.168.1.101" "192.168.1.102")

for device in "${DEVICES[@]}"; do
    echo "Updating device: $device"
    
    # Test connection
    if uv run rauc-updater test --host "$device"; then
        # Deploy bundle
        uv run rauc-updater update "$BUNDLE" --host "$device" --mark-good
        echo "✓ Device $device updated successfully"
    else
        echo "✗ Failed to connect to device $device"
    fi
    
    echo "---"
done
```

## Output and Progress

### Normal Output

```
RAUC Updater Tool
Step 1: Headless CLI Version
Deploy RAUC bundles over SSH/SCP

Starting RAUC Update Process
Bundle: /data/nuc-image-qt5-bundle-intel-corei7-64.raucb
Target: root@192.168.1.100:22

Connecting to root@192.168.1.100:22...
✓ Connected successfully
✓ RAUC found: rauc 1.13

Step 1: Uploading bundle...
Uploading nuc-image-qt5-bundle-intel-corei7-64.raucb to /tmp/nuc-image-qt5-bundle-intel-corei7-64.raucb...
Uploading nuc-image-qt5-bundle-intel-corei7-64.raucb ████████████████████████████████████████ 100.0% • 768.0 MB • 45.2 MB/s • 0:00:00
✓ Upload completed: /tmp/nuc-image-qt5-bundle-intel-corei7-64.raucb

Step 2: Installing bundle...
Installing RAUC bundle: /tmp/nuc-image-qt5-bundle-intel-corei7-64.raucb
Command: rauc install /tmp/nuc-image-qt5-bundle-intel-corei7-64.raucb
⠸ Installing: Processing... ████████████████████████████████████████ 100.0%
✓ RAUC installation completed successfully

Step 3: Cleaning up...
✓ Removed remote file: /tmp/nuc-image-qt5-bundle-intel-corei7-64.raucb

✓ Update completed successfully!
```

### Verbose Output

With `--verbose` flag, additional information is displayed:

- Detailed connection logs
- File transfer progress updates
- Installation progress messages
- System information
- Error details and stack traces

## Error Handling

The tool provides comprehensive error handling:

### Connection Errors

```
✗ Connection error: timed out
✗ Connection failed: Failed to establish SSH connection
```

**Solutions:**
- Check network connectivity
- Verify target device is running
- Confirm SSH service is active
- Check firewall settings

### Authentication Errors

```
✗ Authentication failed
```

**Solutions:**
- Verify username/password
- Check SSH key permissions
- Ensure public key is authorized on target

### RAUC Errors

```
✗ RAUC not found on target system
```

**Solutions:**
- Install RAUC on target device
- Check PATH environment variable
- Verify RAUC service is running

### Bundle Errors

```
✗ Bundle file not found: /path/to/bundle.raucb
✗ Installation failed: Compatible mismatch
```

**Solutions:**
- Verify bundle file exists and is readable
- Check bundle compatibility with target
- Use `--ignore-compatible` if appropriate

## Troubleshooting

### Network Setup

If using the default configuration (192.168.1.100), ensure network interface is configured:

```bash
# Configure network interface (from connect.sh)
sudo ifconfig enp42s0 192.168.1.101 netmask 255.255.255.0 up

# Test connectivity
ping 192.168.1.100
```

### SSH Key Setup

```bash
# Generate SSH key if needed
ssh-keygen -t ed25519 -f ~/.ssh/target_device

# Copy public key to target
ssh-copy-id -i ~/.ssh/target_device.pub root@192.168.1.100

# Test connection
ssh -i ~/.ssh/target_device root@192.168.1.100
```

### Debug Information

```bash
# Enable verbose output
uv run rauc-updater update bundle.raucb --verbose

# Check SSH connection manually
ssh -vvv root@192.168.1.100

# Check RAUC on target
ssh root@192.168.1.100 "rauc --version"
ssh root@192.168.1.100 "rauc status"
```

## Next Steps

This is Step 1 of the RAUC Updater Tool. Future steps will include:

- **Step 2**: Qt6 QML GUI interface
- **Step 3**: Advanced features (multi-device, internationalization)

For development and contribution information, see the main [README.md](../README.md) file.