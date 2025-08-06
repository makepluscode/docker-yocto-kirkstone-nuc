# RAUC Updater Tool

A progressive RAUC (Robust Auto-Update Client) update tool with CLI and GUI interfaces for embedded Linux devices.

## Overview

This tool provides a user-friendly interface for deploying RAUC bundles to target devices over SSH/SCP. It's designed with a three-step development approach:

1. **Step 1**: Headless CLI updater (Current)
2. **Step 2**: Basic Qt6 QML GUI 
3. **Step 3**: Advanced GUI with multi-device support

## Current Features (Step 1)

- ✅ SSH connection to target devices
- ✅ Automatic SSH key setup with password authentication
- ✅ SCP file transfer with progress tracking
- ✅ Remote RAUC installation execution
- ✅ Rich CLI output with status indicators
- ✅ Comprehensive error handling and logging

## Quick Start

### Prerequisites

- Python 3.9 or higher
- uv package manager
- Target device with RAUC installed
- SSH access to target device
- sshpass (for automatic SSH key setup)

### Installation

```bash
cd rauc-updater
uv sync
```

### Usage

```bash
# Setup SSH keys first (one-time setup)
uv run rauc-updater setup-ssh

# Basic usage with default target (192.168.1.100)
uv run rauc-updater update /path/to/bundle.raucb

# Quick update with automatic SSH key setup
uv run rauc-updater update /path/to/bundle.raucb --copy-ssh-key

# Specify custom target
uv run rauc-updater update /path/to/bundle.raucb --host 192.168.1.150

# Custom user and port
uv run rauc-updater update /path/to/bundle.raucb --host 192.168.1.100 --user root --port 22

# Verbose output
uv run rauc-updater update /path/to/bundle.raucb --verbose
```

## Configuration

The tool uses connection parameters from the project's `connect.sh` as defaults:
- **Default Host**: 192.168.1.100
- **Default User**: root  
- **Default Port**: 22

## Development

### Project Structure

```
rauc-updater/
├── src/
│   ├── cli.py          # CLI interface and commands
│   ├── utils.py        # Utility functions
│   ├── core/           # Step 1: Core functionality
│   │   ├── connection.py    # SSH connection and key management
│   │   ├── transfer.py      # File transfer with progress
│   │   └── installer.py     # RAUC installation
│   ├── gui/            # Step 2: GUI components (future)
│   └── advanced/       # Step 3: Advanced features (future)
├── tests/              # Unit tests
├── docs/               # Documentation
└── resources/          # QML files and resources (future)
```

### Running Tests

```bash
uv run pytest
```

### Code Formatting

```bash
uv run black src/
uv run isort src/
```

## Target Device Requirements

- Linux system with RAUC installed
- SSH server running
- Sufficient disk space for bundle
- Appropriate permissions for RAUC operations

## Error Handling

The tool provides comprehensive error handling for:
- Network connectivity issues
- Authentication failures
- File transfer errors
- RAUC installation failures
- Insufficient disk space

## Roadmap

### Step 2: GUI Development
- Qt6 QML user interface
- File selection dialogs
- Visual progress indicators
- Real-time log display

### Step 3: Advanced Features
- Multi-device management
- System status monitoring
- Good-mark processing
- Internationalization (Korean/English)
- Configuration management

## Contributing

This is a proof-of-concept tool for RAUC integration testing. Contributions are welcome for bug fixes and feature enhancements.

## License

MIT License - see LICENSE file for details.