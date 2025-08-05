# RAUC Updater Tool

A Python-based tool for deploying RAUC bundles to target devices over SSH/SCP.

## Features

- **SSH Connection Management**: Secure connection to target devices
- **RAUC Bundle Deployment**: Upload and install RAUC bundles
- **Status Monitoring**: Check RAUC system status and slot information
- **Progress Tracking**: Real-time progress updates during operations
- **Cleanup Management**: Remove temporary files after operations
- **CLI Interface**: Command-line interface for automation

## Installation

### Prerequisites

- Python 3.9 or higher
- Target device with SSH access
- RAUC installed on target device

### Setup

1. **Clone the repository**:
   ```bash
   cd updater/rauc-updater
   ```

2. **Activate virtual environment**:
   ```bash
   source .venv/bin/activate
   ```

3. **Install dependencies**:
   ```bash
   pip install -e .
   ```

## Usage

### Basic Commands

#### 1. Test Connection
Test connection to target device and verify RAUC availability:

```bash
python -m rauc_updater.cli test --host 192.168.1.100 --user root --password [PASSWORD]
```

**Options**:
- `--host`: Target host IP address (default: 192.168.1.100)
- `--user`: SSH username (default: root)
- `--port`: SSH port (default: 22)
- `--password`: SSH password (not recommended for production)
- `--key-file`: SSH private key file path

#### 2. Check RAUC Status
Display current RAUC system status and slot information:

```bash
python -m rauc_updater.cli status --host 192.168.1.100 --user root --password [PASSWORD]
```

#### 3. Update Device
Upload and install RAUC bundle on target device:

```bash
python -m rauc_updater.cli update bundle.raucb --host 192.168.1.100 --user root --password [PASSWORD]
```

**Options**:
- `--remote-path`: Remote directory for bundle upload (default: /tmp)
- `--timeout`: Installation timeout in seconds (default: 600)
- `--ignore-compatible`: Ignore compatibility checks
- `--mark-good`: Mark slot as good after installation
- `--verbose`: Verbose output
- `--cleanup`: Remove bundle after installation (default: True)

#### 4. Cleanup Temporary Files
Remove temporary bundle files from target device:

```bash
python -m rauc_updater.cli cleanup --host 192.168.1.100 --user root --password [PASSWORD]
```

## Testing

### 1. Environment Setup

Ensure you have access to a target device with RAUC installed:

```bash
# Check target connectivity
ping -c 3 192.168.1.100

# Verify SSH access
ssh root@192.168.1.100 "rauc --version"
```

### 2. Connection Test

Test basic connectivity and RAUC availability:

```bash
# Basic connection test
python -m rauc_updater.cli test --host 192.168.1.100 --user root --password [PASSWORD]

# Expected output:
# ✓ Connected successfully
# ✓ RAUC found: rauc 1.13
# ✓ Connection test successful!
```

### 3. Status Check

Verify RAUC system status:

```bash
python -m rauc_updater.cli status --host 192.168.1.100 --user root --password [PASSWORD]

# Expected output shows:
# - System compatibility
# - Boot slot information
# - Slot states (A/B)
```

### 4. Update Process Test

Test the complete update workflow:

```bash
# Create test bundle (for testing only)
echo "Test bundle content" > test-bundle.raucb

# Test update process
python -m rauc_updater.cli update test-bundle.raucb \
  --host 192.168.1.100 \
  --user root \
  --password [PASSWORD] \
  --timeout 30 \
  --verbose

# Note: Test bundle will fail installation (expected)
# But upload and verification steps should succeed
```

### 5. Cleanup Test

Test cleanup functionality:

```bash
python -m rauc_updater.cli cleanup --host 192.168.1.100 --user root --password [PASSWORD]
```

### 6. Unit Tests

Run the test suite:

```bash
# Run all tests
python -m pytest tests/ -v

# Expected: 8 tests passed
```

## Target System Requirements

### Hardware
- Intel NUC (Core i7 x64) or compatible device
- Network connectivity to host system

### Software
- Yocto Linux with RAUC support
- RAUC version 1.13 or higher
- SSH server enabled
- Sufficient disk space for bundle installation

### Network Configuration
- Target IP: 192.168.1.100
- Host IP: 192.168.1.101
- SSH port: 22

## Security Considerations

### Authentication
- **Recommended**: Use SSH key authentication
- **Development**: Password authentication (change default password)
- **Production**: Implement proper key management

### Network Security
- Use secure network connections
- Restrict SSH access to specific IP addresses
- Consider using non-standard SSH ports

### Best Practices
- Never commit passwords to version control
- Regularly update target system security patches
- Monitor system logs for unauthorized access

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
```

## Development

### Project Structure
```
rauc-updater/
├── src/rauc_updater/
│   ├── cli.py              # Command-line interface
│   ├── core/
│   │   ├── connection.py   # SSH connection management
│   │   ├── installer.py    # RAUC installation logic
│   │   └── transfer.py     # File transfer operations
│   ├── gui/                # GUI interface (future)
│   └── utils.py            # Utility functions
├── tests/                  # Test suite
├── docs/                   # Documentation
└── pyproject.toml          # Project configuration
```

### Adding New Features
1. Create feature branch
2. Implement functionality
3. Add tests
4. Update documentation
5. Submit pull request

## License

MIT License - see LICENSE file for details.

## Support

For issues and questions:
- Check the troubleshooting section
- Review target system requirements
- Ensure proper network configuration
- Verify RAUC installation on target device
