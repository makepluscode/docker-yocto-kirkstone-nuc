#!/bin/bash

set -e

echo "🚀 Deploying RAUC bundle to NUC target..."

# Default configuration
DEFAULT_TARGET="192.168.1.100"
DEFAULT_USER="root"
REMOTE_DIR="/tmp"
CONNECT_TIMEOUT=10
BUNDLE_PATTERN="*.raucb"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] [TARGET] [BUNDLE_FILE]"
    echo ""
    echo "Deploy RAUC bundle to NUC target device"
    echo ""
    echo "Arguments:"
    echo "  TARGET         Target IP address (default: $DEFAULT_TARGET)"
    echo "  BUNDLE_FILE    Specific bundle file to deploy (default: latest *.raucb)"
    echo ""
    echo "Options:"
    echo "  -u, --user USER     SSH username (default: $DEFAULT_USER)"
    echo "  -d, --dir DIR       Remote directory (default: $REMOTE_DIR)"
    echo "  -t, --timeout SEC   Connection timeout (default: $CONNECT_TIMEOUT)"
    echo "  -i, --install       Install bundle after copying"
    echo "  -s, --status        Check RAUC status after installation"
    echo "  -v, --verbose       Enable verbose output"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Deploy to default target"
    echo "  $0 192.168.1.150                     # Deploy to specific IP"
    echo "  $0 -i 192.168.1.100                  # Deploy and install"
    echo "  $0 -u admin -d /data 192.168.1.100   # Custom user and directory"
    echo "  $0 bundle.raucb                      # Deploy specific bundle"
    echo "  $0 -i -s 192.168.1.100 bundle.raucb  # Full deployment with status check"
}

# Parse command line options
INSTALL_BUNDLE=false
CHECK_STATUS=false
VERBOSE=false
TARGET=""
BUNDLE_FILE=""
SSH_USER="$DEFAULT_USER"
REMOTE_TARGET_DIR="$REMOTE_DIR"

while [[ $# -gt 0 ]]; do
    case $1 in
        -u|--user)
            SSH_USER="$2"
            shift 2
            ;;
        -d|--dir)
            REMOTE_TARGET_DIR="$2"
            shift 2
            ;;
        -t|--timeout)
            CONNECT_TIMEOUT="$2"
            shift 2
            ;;
        -i|--install)
            INSTALL_BUNDLE=true
            shift
            ;;
        -s|--status)
            CHECK_STATUS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        -*)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
        *)
            if [ -z "$TARGET" ]; then
                # Check if argument is a bundle file or IP address
                if [[ "$1" =~ \.raucb$ ]]; then
                    BUNDLE_FILE="$1"
                    TARGET="$DEFAULT_TARGET"
                else
                    TARGET="$1"
                fi
            elif [ -z "$BUNDLE_FILE" ]; then
                BUNDLE_FILE="$1"
            else
                print_error "Too many arguments"
                show_usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Set default target if not specified
if [ -z "$TARGET" ]; then
    TARGET="$DEFAULT_TARGET"
fi

# Build SSH target string
SSH_TARGET="$SSH_USER@$TARGET"

print_status "=== RAUC Bundle Deployment ==="
print_status "Target: $SSH_TARGET"
print_status "Remote directory: $REMOTE_TARGET_DIR"
print_status "Connection timeout: ${CONNECT_TIMEOUT}s"

# Step 1: Find bundle file to deploy
if [ -z "$BUNDLE_FILE" ]; then
    print_status "Step 1: Finding latest RAUC bundle..."
    
    # Look for bundles in current directory and build output
    BUNDLE_FILES=$(find . -name "$BUNDLE_PATTERN" -type f 2>/dev/null | head -10)
    
    # Also check in project root
    if [ -z "$BUNDLE_FILES" ]; then
        BUNDLE_FILES=$(find ../../ -maxdepth 1 -name "$BUNDLE_PATTERN" -type f 2>/dev/null | head -10)
    fi
    
    # Check in build output directory
    if [ -z "$BUNDLE_FILES" ]; then
        BUNDLE_FILES=$(find build/output/ -name "$BUNDLE_PATTERN" -type f 2>/dev/null | head -10)
    fi
    
    if [ -z "$BUNDLE_FILES" ]; then
        print_error "No RAUC bundle files found (*.raucb)"
        print_status "Please specify bundle file explicitly or ensure bundle exists"
        exit 1
    fi
    
    # Use the most recent bundle
    BUNDLE_FILE=$(echo "$BUNDLE_FILES" | head -1)
    print_success "Found bundle: $BUNDLE_FILE"
else
    print_status "Step 1: Using specified bundle: $BUNDLE_FILE"
    
    if [ ! -f "$BUNDLE_FILE" ]; then
        print_error "Bundle file not found: $BUNDLE_FILE"
        exit 1
    fi
fi

# Get bundle info
BUNDLE_SIZE=$(du -h "$BUNDLE_FILE" | cut -f1)
BUNDLE_NAME=$(basename "$BUNDLE_FILE")
REMOTE_BUNDLE_PATH="$REMOTE_TARGET_DIR/$BUNDLE_NAME"

print_status "Bundle size: $BUNDLE_SIZE"
print_status "Remote path: $REMOTE_BUNDLE_PATH"

# Step 2: Test SSH connectivity
print_status "Step 2: Testing SSH connectivity..."

if ! timeout $CONNECT_TIMEOUT ssh -o ConnectTimeout=$CONNECT_TIMEOUT -o BatchMode=yes "$SSH_TARGET" true 2>/dev/null; then
    print_error "Cannot connect to $SSH_TARGET"
    print_status "Please ensure:"
    echo "  - Target device is powered on and connected"
    echo "  - SSH service is running on target"
    echo "  - SSH keys are properly configured"
    echo "  - Network connectivity is available"
    echo ""
    print_status "Test connectivity manually:"
    echo "  ssh $SSH_TARGET"
    exit 1
fi

print_success "SSH connectivity confirmed"

# Step 3: Copy bundle to target
print_status "Step 3: Copying bundle to target ($BUNDLE_SIZE)..."

if $VERBOSE; then
    SCP_VERBOSE="-v"
else
    SCP_VERBOSE=""
fi

if ! scp $SCP_VERBOSE -o ConnectTimeout=$CONNECT_TIMEOUT "$BUNDLE_FILE" "$SSH_TARGET:$REMOTE_BUNDLE_PATH"; then
    print_error "Failed to copy bundle to target"
    exit 1
fi

print_success "Bundle copied successfully to: $REMOTE_BUNDLE_PATH"

# Step 4: Verify bundle on target
print_status "Step 4: Verifying bundle on target..."

REMOTE_SIZE=$(ssh -o ConnectTimeout=$CONNECT_TIMEOUT "$SSH_TARGET" "du -h '$REMOTE_BUNDLE_PATH'" 2>/dev/null | cut -f1 || echo "unknown")
if [ "$REMOTE_SIZE" != "unknown" ]; then
    print_success "Bundle verified on target (Size: $REMOTE_SIZE)"
else
    print_warning "Could not verify bundle size on target"
fi

# Step 5: Install bundle (optional)
if $INSTALL_BUNDLE; then
    print_status "Step 5: Installing RAUC bundle on target..."
    
    if ssh -o ConnectTimeout=$CONNECT_TIMEOUT "$SSH_TARGET" "rauc install '$REMOTE_BUNDLE_PATH'"; then
        print_success "Bundle installation completed"
        
        # Step 6: Check status (optional)
        if $CHECK_STATUS; then
            print_status "Step 6: Checking RAUC status..."
            echo ""
            ssh -o ConnectTimeout=$CONNECT_TIMEOUT "$SSH_TARGET" "rauc status"
            echo ""
            print_success "RAUC status check completed"
        fi
    else
        print_error "Bundle installation failed"
        exit 1
    fi
else
    print_warning "Bundle copied but not installed (use -i to install)"
fi

# Step 7: Display summary and next steps
echo ""
print_success "=== Deployment Summary ==="
echo "📁 Local bundle: $BUNDLE_FILE ($BUNDLE_SIZE)"
echo "🎯 Target: $SSH_TARGET"
echo "📍 Remote path: $REMOTE_BUNDLE_PATH"
echo "✅ Status: Bundle deployed successfully"

if ! $INSTALL_BUNDLE; then
    echo ""
    print_status "Manual installation commands:"
    echo "  ssh $SSH_TARGET"
    echo "  rauc install $REMOTE_BUNDLE_PATH"
    echo "  rauc status"
fi

echo ""
print_status "Additional useful commands:"
echo "  # Check bundle info on target"
echo "  ssh $SSH_TARGET 'rauc info $REMOTE_BUNDLE_PATH'"
echo ""
echo "  # Monitor installation progress"
echo "  ssh $SSH_TARGET 'journalctl -u rauc -f'"
echo ""
echo "  # Clean up bundle after installation"
echo "  ssh $SSH_TARGET 'rm $REMOTE_BUNDLE_PATH'"

print_success "🎉 RAUC bundle deployment completed!"