# Target System Information

## Hardware Configuration

- **Device**: Intel NUC (Next Unit of Computing)
- **Architecture**: Intel Core i7 x64
- **OS**: Yocto Linux with RAUC support
- **RAUC Version**: 1.13

## Network Configuration

### Target System
- **IP Address**: `192.168.1.100`
- **SSH Port**: `22`
- **Username**: `root`
- **Password**: `[Contact system administrator]`

### Host System
- **IP Address**: `192.168.1.101`
- **Network Interface**: `enp42s0`
- **Subnet Mask**: `255.255.255.0`

## Connection Methods

### 1. Quick Connect Script
```bash
# From project root directory
./connect.sh
```

### 2. Manual SSH Connection
```bash
# Set network interface
sudo ifconfig enp42s0 192.168.1.101 netmask 255.255.255.0 up

# Connect via SSH
ssh root@192.168.1.100
```

### 3. Using RAUC Updater Tool
```bash
# Test connection
python -m rauc_updater.cli test --host 192.168.1.100 --user root --password [PASSWORD]

# Check RAUC status
python -m rauc_updater.cli status --host 192.168.1.100 --user root --password [PASSWORD]

# Update with bundle
python -m rauc_updater.cli update bundle.raucb --host 192.168.1.100 --user root --password [PASSWORD]

# Cleanup temporary files
python -m rauc_updater.cli cleanup --host 192.168.1.100 --user root --password [PASSWORD]
```

## RAUC System Status

### Current Configuration
- **Compatible**: `intel-i7-x64-nuc-rauc`
- **Booted from**: `rootfs.0 (A)`
- **Activated slot**: `rootfs.0 (A)`

### Slot Information
- **Slot A** (`/dev/sda2`): 
  - Status: `booted` (currently active)
  - Boot status: `good`
  - Mounted: `/`

- **Slot B** (`/dev/sda3`):
  - Status: `inactive`
  - Boot status: `bad`

## Setup Instructions

### 1. Network Setup
```bash
# Ensure network interface is configured
sudo ifconfig enp42s0 192.168.1.101 netmask 255.255.255.0 up

# Test connectivity
ping -c 3 192.168.1.100
```

### 2. SSH Key Setup (Optional)
```bash
# Generate SSH key (if not exists)
ssh-keygen -t rsa -b 4096 -f ~/.ssh/id_rsa -N ""

# Copy public key to target
ssh-copy-id -i ~/.ssh/id_rsa.pub root@192.168.1.100

# Test key-based authentication
ssh root@192.168.1.100 "echo 'SSH key authentication successful'"
```

### 3. RAUC Updater Environment
```bash
# Navigate to updater directory
cd updater/rauc-updater

# Activate virtual environment
source .venv/bin/activate

# Install dependencies (if needed)
pip install -e .
```

## Troubleshooting

### Connection Issues
1. **Network connectivity**: Check if both devices are on the same network
2. **SSH service**: Ensure SSH service is running on target
3. **Firewall**: Check for any firewall blocking port 22

### RAUC Issues
1. **RAUC not found**: Ensure RAUC is installed on target system
2. **Permission denied**: Check file permissions on target
3. **Bundle compatibility**: Verify bundle is compatible with target system

### Common Commands
```bash
# Check target system status
ssh root@192.168.1.100 "rauc status"

# Check disk space
ssh root@192.168.1.100 "df -h"

# Check RAUC version
ssh root@192.168.1.100 "rauc --version"

# List installed packages
ssh root@192.168.1.100 "rpm -qa | grep rauc"
```

## Security Notes

⚠️ **IMPORTANT**: For security reasons, password authentication is not recommended.

### Recommended Authentication Methods:
1. **SSH Key Authentication** (Recommended)
   - Generate SSH key pair
   - Copy public key to target system
   - Use key-based authentication for all connections

2. **Password Authentication** (Development only)
   - Use only in development/testing environments
   - Change default password immediately
   - Never commit passwords to version control

### Security Best Practices:
- Use SSH keys instead of password authentication
- Ensure network is properly secured
- Regular security updates should be applied
- Restrict SSH access to specific IP addresses
- Use non-standard SSH port in production

## Development Notes

- Target system is used for testing RAUC updater functionality
- Supports A/B boot system for safe updates
- Compatible with Yocto-generated RAUC bundles
- Ideal for embedded system development and testing 