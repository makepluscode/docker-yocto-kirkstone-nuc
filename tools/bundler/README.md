# RAUC Bundler

A minimal command-line tool for creating RAUC bundles from manifest files.

## Overview

The RAUC Bundler is a simple wrapper around the `rauc bundle` command that provides additional validation and convenience features for creating RAUC update bundles.

## Features

- Validate manifest and certificate files before bundling
- Check output directory existence
- Prevent accidental overwrites (with force option)
- Verbose output mode
- Comprehensive error handling
- Command-line argument validation

## Requirements

- GCC compiler
- RAUC tools installed on the system
- Make (for building)

## Installation

### Build from source

```bash
# Clone or download the source
cd bundler

# Build the executable
make

# Install system-wide (optional)
sudo make install
```

### Development

```bash
# Build with debug symbols
make debug

# Build with strict warnings
make check

# Format code (requires clang-format)
make format

# Clean build artifacts
make clean
```

## Usage

### Basic Usage

```bash
# Create a bundle from manifest
./bundler manifest.raucm bundle.raucb

# Create a signed bundle
./bundler -c cert.pem -k key.pem manifest.raucm bundle.raucb
```

### Command Line Options

```
Usage: bundler [OPTIONS] <manifest> <output>

Arguments:
  manifest    Path to the RAUC manifest file
  output      Path for the output .raucb bundle

Options:
  -c, --cert PATH    Path to certificate file
  -k, --key PATH     Path to private key file
  -v, --verbose      Enable verbose output
  -f, --force        Overwrite existing output file
  -h, --help         Show this help message
```

### Examples

```bash
# Basic bundle creation
./bundler my-update.raucm update-bundle.raucb

# Signed bundle with verbose output
./bundler -v -c /path/to/cert.pem -k /path/to/key.pem my-update.raucm signed-bundle.raucb

# Force overwrite existing bundle
./bundler -f manifest.raucm bundle.raucb

# Show help
./bundler --help
```

## Error Handling

The bundler performs several validation checks:

- **Manifest file existence**: Ensures the manifest file exists before attempting to create the bundle
- **Certificate/key validation**: Verifies both certificate and key files exist when signing
- **Output directory**: Checks if the output directory exists
- **File overwrite protection**: Prevents accidental overwrites unless `-f` is specified
- **RAUC command execution**: Reports detailed error messages from the underlying `rauc bundle` command

## Exit Codes

- `0`: Success
- `1`: Error (invalid arguments, file not found, etc.)
- Other: Exit code from the underlying `rauc bundle` command

## Integration

The bundler can be easily integrated into build systems and CI/CD pipelines:

```bash
# In a Makefile
bundle: $(MANIFEST)
	./bundler -v -c $(CERT) -k $(KEY) $(MANIFEST) $(OUTPUT)

# In a shell script
if ./bundler -c cert.pem -k key.pem manifest.raucm bundle.raucb; then
    echo "Bundle created successfully"
else
    echo "Bundle creation failed"
    exit 1
fi
```

## License

This project is provided as-is for educational and development purposes.

## Contributing

Feel free to submit issues and enhancement requests. 