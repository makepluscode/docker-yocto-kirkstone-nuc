# RAUC Bundle Update Guide for Intel NUC

This guide provides comprehensive instructions for creating RAUC update bundles for the Intel NUC Yocto project and deploying them to target devices for A/B boot system updates.

## Overview

RAUC (Robust Auto-Update Client) provides a reliable way to update embedded Linux systems using A/B partitioning. This setup allows atomic updates with automatic rollback capabilities, ensuring system reliability during updates.

### System Architecture

- **Target Platform**: Intel NUC with SATA SSD storage
- **Boot System**: GRUB EFI with A/B partitions
- **Base Image**: `nuc-image-qt5` (Qt5-enabled Linux image)
- **Update Mechanism**: RAUC bundles (`.raucb` files)
- **Storage Layout**: A/B partitions on `/dev/sda`

## Prerequisites

### Host System Requirements

- Docker installed and running
- Network connectivity to target device
- SSH access to target device
- Sufficient disk space for builds (~30GB minimum)

### Target System Requirements

- Intel NUC running with A/B boot system
- RAUC service running on target
- Network connectivity
- SSH access enabled

## Building RAUC Bundles

### Method 1: Automated Bundle Build

The simplest way to build a RAUC bundle:

```bash
./build.sh bundle
```

This command will:
1. Clean previous build configurations
2. Set up the Docker build environment
3. Configure Yocto layers and dependencies
4. Build the `nuc-image-qt5-bundle` recipe
5. Create the `.raucb` bundle file

### Method 2: Manual Bundle Build

For development and debugging:

```bash
# Enter manual build mode
./build.sh manual

# Inside the container, build the bundle
bitbake nuc-image-qt5-bundle
```

### Build Script Options

- `./build.sh` - Default auto build (builds nuc-image-qt5)
- `./build.sh auto` - Same as default
- `./build.sh manual` - Enter container for manual operations
- `./build.sh bundle` - Build RAUC bundle for nuc-image-qt5

## Bundle Artifacts

After a successful build, the bundle will be located at:

```
kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-bundle-intel-corei7-64.raucb
```

### Bundle Information

- **File Extension**: `.raucb`
- **Content**: Complete rootfs for nuc-image-qt5
- **Filesystem**: ext4
- **Signature**: Signed with development keys
- **Size**: Typically 1-2GB (depends on image content)

## Deploying Bundles to Target

### Method 1: Automated Deployment Script

Use the provided deployment script for easy transfer:

```bash
# Deploy to target at 192.168.1.100 (default IP)
./deploy_rauc_bundle.sh

# Deploy to specific IP with custom user
./deploy_rauc_bundle.sh 192.168.1.150 nucuser
```

The script will:
1. Verify bundle exists locally
2. Test network connectivity to target
3. Copy bundle via SCP to `/tmp` on target
4. Verify successful transfer
5. Provide installation instructions

### Method 2: Manual Deployment

Copy the bundle manually:

```bash
# Copy bundle to target
scp kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/nuc-image-qt5-bundle-intel-corei7-64.raucb \
    nuc@192.168.1.100:/tmp/

# Verify transfer
ssh nuc@192.168.1.100 "ls -la /tmp/*.raucb"
```

## Installing Updates on Target

### Prerequisites Check

Before installing, verify RAUC is properly configured:

```bash
# Check RAUC status
rauc status

# Check current boot slot
rauc status --detailed

# Verify system configuration
sudo rauc info /etc/rauc/system.conf
```

### Installing the Update

```bash
# Install the RAUC bundle
sudo rauc install /tmp/nuc-image-qt5-bundle-intel-corei7-64.raucb
```

### Installation Process

1. **Verification**: RAUC verifies bundle signature and compatibility
2. **Target Selection**: Automatically selects inactive slot (A or B)
3. **Installation**: Writes new rootfs to inactive partition
4. **Boot Configuration**: Updates GRUB to boot from new partition
5. **Completion**: Marks installation complete

### Post-Installation

```bash
# Check installation status
rauc status

# Reboot to activate new system
sudo reboot
```

## Verification and Rollback

### After Reboot

```bash
# Verify current boot slot
rauc status --detailed

# Check system version/build info
cat /etc/os-release

# Mark boot as good (prevents automatic rollback)
sudo rauc status mark-good
```

### Manual Rollback

If the update is problematic:

```bash
# Mark current boot as bad and rollback
sudo rauc status mark-bad

# Reboot to previous version
sudo reboot
```

### Automatic Rollback

The system automatically rolls back if:
- Boot fails completely
- System doesn't mark boot as "good" within timeout
- Critical services fail to start

## Troubleshooting

### Build Issues

**Bundle recipe not found:**
```bash
# Verify layer configuration
bitbake-layers show-layers | grep meta-nuc

# Check recipe exists
bitbake-layers show-recipes nuc-image-qt5-bundle
```

**Build failures:**
```bash
# Clean and rebuild
bitbake -c cleansstate nuc-image-qt5-bundle
bitbake nuc-image-qt5-bundle
```

### Deployment Issues

**Network connectivity:**
```bash
# Test connection
ping 192.168.1.100

# Test SSH access
ssh nuc@192.168.1.100 echo "Connected"
```

**Bundle transfer failures:**
```bash
# Check available space on target
ssh nuc@192.168.1.100 "df -h /tmp"

# Manual copy with verbose output
scp -v bundle.raucb nuc@192.168.1.100:/tmp/
```

### Installation Issues

**RAUC service not running:**
```bash
# Check service status
sudo systemctl status rauc

# Start RAUC service
sudo systemctl start rauc
sudo systemctl enable rauc
```

**Signature verification failures:**
```bash
# Check RAUC configuration
sudo rauc info /etc/rauc/system.conf

# Verify bundle signature manually
rauc info /tmp/bundle.raucb
```

**Insufficient space:**
```bash
# Check partition sizes
df -h

# Clean old files
sudo rauc status mark-active
```

## Security Considerations

### Production Deployment

For production systems:

1. **Generate proper certificates:**
   ```bash
   # Don't use development keys in production
   # Generate proper CA and device certificates
   ```

2. **Secure bundle distribution:**
   - Use HTTPS for bundle downloads
   - Implement proper authentication
   - Use secure storage for bundles

3. **Network security:**
   - Use VPN or secure networks for deployment
   - Implement proper firewall rules
   - Use SSH key authentication

### Key Management

- Development keys are in `kirkstone/build/example-ca/`
- Never use development keys in production
- Rotate certificates regularly
- Implement proper key backup and recovery

## Advanced Configuration

### Custom Bundle Content

Modify `nuc-image-qt5-bundle.bb` to customize bundle content:

```bash
# Edit bundle recipe
vim kirkstone/meta-nuc/recipes-core/bundles/nuc-image-qt5-bundle.bb
```

### Multiple Boot Configurations

The system supports multiple partition layouts. Configure in:
```bash
kirkstone/meta-nuc/recipes-core/rauc/files/intel-corei7-64/system.conf
```

### Automated Deployment

For automated deployment pipelines:

```bash
# Example CI/CD integration
#!/bin/bash
BUILD_OUTPUT=$(./build.sh bundle)
if [ $? -eq 0 ]; then
    ./deploy_rauc_bundle.sh $TARGET_IP
    ssh $TARGET_IP "sudo rauc install /tmp/*.raucb && sudo reboot"
fi
```

## Monitoring and Logging

### Build Logs

Monitor build progress:
```bash
# Follow build logs
tail -f kirkstone/build/tmp-glibc/work/intel_corei7_64-oe-linux/nuc-image-qt5-bundle/*/temp/log.do_bundle
```

### Target Logs

Monitor installation on target:
```bash
# Follow RAUC logs
sudo journalctl -f -u rauc

# System boot logs
sudo journalctl -b
```

## File Locations Reference

### Host System

- **Build Script**: `./build.sh`
- **Deployment Script**: `./deploy_rauc_bundle.sh`
- **Bundle Recipe**: `kirkstone/meta-nuc/recipes-core/bundles/nuc-image-qt5-bundle.bb`
- **Bundle Output**: `kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64/*.raucb`
- **Build Logs**: `kirkstone/build/tmp-glibc/work/*/temp/`

### Target System

- **RAUC Config**: `/etc/rauc/system.conf`
- **Bundle Storage**: `/tmp/` (temporary)
- **Boot Config**: `/boot/grub/grub.cfg`
- **System Info**: `/etc/os-release`
- **Logs**: `/var/log/` and `journalctl`

## Summary

This guide provides a complete workflow for:

1. **Building** RAUC bundles with `./build.sh bundle`
2. **Deploying** bundles with `./deploy_rauc_bundle.sh`
3. **Installing** updates with `sudo rauc install`
4. **Verifying** updates with `rauc status`
5. **Managing** rollbacks if needed

The A/B boot system ensures reliable updates with automatic rollback capabilities, making it suitable for production embedded systems requiring high reliability.