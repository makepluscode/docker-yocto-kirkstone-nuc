#!/bin/bash

# Updater Server Build Script
# Builds Python backend and manages certificates

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
CERTS_DIR="$PROJECT_ROOT/certs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
Updater Server Build Script

Usage: $0 [COMMAND] [OPTIONS]

Commands:
    all         Build everything (default)
    backend     Build Python backend only
    certs       Generate SSL certificates
    clean       Clean build artifacts
    install     Install dependencies
    test        Run tests
    package     Create distribution package

Options:
    --debug         Build in debug mode
    --release       Build in release mode (default)
    --verbose       Show verbose output
    --help          Show this help message

Examples:
    $0                      # Build everything
    $0 backend              # Build backend only
    $0 certs                # Generate certificates
    $0 clean                # Clean all build artifacts
    $0 install              # Install all dependencies

EOF
}

# Check dependencies
check_dependencies() {
    log_info "Checking dependencies..."
    
    local missing_deps=()
    
    # Check Python and uv
    if ! command -v python3 &> /dev/null; then
        missing_deps+=("python3")
    fi
    
    if ! command -v uv &> /dev/null; then
        log_warning "uv not found, will try pip instead"
    fi
    
    # Check OpenSSL for certificates
    if [[ "$BUILD_CERTS" == "true" ]]; then
        if ! command -v openssl &> /dev/null; then
            missing_deps+=("openssl")
        fi
    fi
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_error "Missing dependencies: ${missing_deps[*]}"
        log_info "Please install missing dependencies and try again"
        exit 1
    fi
    
    log_success "All dependencies available"
}

# Install Python dependencies
install_backend_deps() {
    log_info "Installing Python backend dependencies..."
    
    cd "$PROJECT_ROOT"
    
    if command -v uv &> /dev/null; then
        log_info "Using uv to install dependencies"
        uv sync
    else
        log_info "Using pip to install dependencies"
        if [[ -f "requirements.txt" ]]; then
            pip3 install -r requirements.txt
        else
            log_error "requirements.txt not found and uv not available"
            exit 1
        fi
    fi
    
    log_success "Backend dependencies installed"
}

# Build Python backend
build_backend() {
    log_info "Building Python backend..."
    
    cd "$PROJECT_ROOT"
    
    # Install dependencies
    install_backend_deps
    
    # Validate Python modules
    log_info "Validating Python modules..."
    if command -v uv &> /dev/null; then
        uv run python -c "import updater_server.main; print('Backend modules validated')"
    else
        python3 -c "import updater_server.main; print('Backend modules validated')"
    fi
    
    log_success "Backend build completed"
}

# Generate SSL certificates
generate_certificates() {
    log_info "Generating SSL certificates..."
    
    cd "$PROJECT_ROOT"
    
    if [[ -f "generate_certs.sh" ]]; then
        chmod +x generate_certs.sh
        ./generate_certs.sh
    else
        log_error "generate_certs.sh not found"
        exit 1
    fi
    
    log_success "SSL certificates generated"
}

# Clean build artifacts
clean_build() {
    log_info "Cleaning build artifacts..."
    
    # Clean Python cache
    log_info "Cleaning Python cache files..."
    find "$PROJECT_ROOT" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
    find "$PROJECT_ROOT" -type f -name "*.pyc" -delete 2>/dev/null || true
    
    # Clean temporary files
    find "$PROJECT_ROOT" -type f -name "*.tmp" -delete 2>/dev/null || true
    find "$PROJECT_ROOT" -type f -name ".DS_Store" -delete 2>/dev/null || true
    
    # Clean logs
    rm -f "$PROJECT_ROOT"/*.log 2>/dev/null || true
    rm -rf "$PROJECT_ROOT/logs" 2>/dev/null || true
    
    log_success "Build artifacts cleaned"
}

# Run tests
run_tests() {
    log_info "Running tests..."
    
    cd "$PROJECT_ROOT"
    
    # Test backend
    log_info "Testing backend..."
    if [[ -f "scripts/test_client.py" ]]; then
        python3 scripts/test_client.py
    else
        log_warning "No backend tests found"
    fi
    
    # Test refactored components
    if [[ -f "scripts/test_refactored.py" ]]; then
        log_info "Testing refactored components..."
        python3 scripts/test_refactored.py
    else
        log_warning "No refactored tests found"
    fi
    
    log_success "Tests completed"
}

# Create distribution package
create_package() {
    log_info "Creating distribution package..."
    
    PACKAGE_DIR="$PROJECT_ROOT/dist"
    PACKAGE_NAME="updater-server-$(date +%Y%m%d)"
    
    mkdir -p "$PACKAGE_DIR/$PACKAGE_NAME"
    
    # Copy backend files
    cp -r updater_server "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp pyproject.toml "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp server.sh "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp updater.py "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp run.sh "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp generate_certs.sh "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp README.md "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp REFACTORING_GUIDE.md "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp -r scripts "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp -r bundle "$PACKAGE_DIR/$PACKAGE_NAME/"
    
    # Copy certificates if they exist
    if [[ -d "certs" ]]; then
        cp -r certs "$PACKAGE_DIR/$PACKAGE_NAME/"
    fi
    
    # Create archive
    cd "$PACKAGE_DIR"
    tar -czf "$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"
    
    log_success "Package created: $PACKAGE_DIR/$PACKAGE_NAME.tar.gz"
}

# Install system dependencies (helper)
install_system_deps() {
    log_info "Installing system dependencies..."
    
    if command -v apt &> /dev/null; then
        log_info "Detected Debian/Ubuntu system"
        sudo apt update
        sudo apt install -y python3 python3-pip python3-pyqt6 python3-requests openssl curl
    elif command -v yum &> /dev/null; then
        log_info "Detected RHEL/CentOS system"
        sudo yum install -y python3 python3-pip python3-pyqt6 python3-requests openssl curl
    elif command -v dnf &> /dev/null; then
        log_info "Detected Fedora system"
        sudo dnf install -y python3 python3-pip python3-pyqt6 python3-requests openssl curl
    elif command -v brew &> /dev/null; then
        log_info "Detected macOS with Homebrew"
        brew install python3 pyqt6 openssl curl
    else
        log_warning "Unknown system - please install dependencies manually"
        log_info "Required: python3, python3-pyqt6, python3-requests, openssl, curl"
    fi
    
    # Install uv
    if ! command -v uv &> /dev/null; then
        log_info "Installing uv..."
        curl -LsSf https://astral.sh/uv/install.sh | sh
        source ~/.bashrc || true
    fi
    
    log_success "System dependencies installed"
}

# Parse command line arguments
BUILD_BACKEND="false"
BUILD_CERTS="false"
BUILD_TYPE="Release"
VERBOSE="false"
COMMAND=""

while [[ $# -gt 0 ]]; do
    case $1 in
        all)
            BUILD_BACKEND="true"
            BUILD_CERTS="true"
            COMMAND="all"
            shift
            ;;
        backend)
            BUILD_BACKEND="true"
            COMMAND="backend"
            shift
            ;;
        certs)
            BUILD_CERTS="true"
            COMMAND="certs"
            shift
            ;;
        clean)
            COMMAND="clean"
            shift
            ;;
        install)
            COMMAND="install"
            shift
            ;;
        test)
            COMMAND="test"
            shift
            ;;
        package)
            COMMAND="package"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --verbose)
            VERBOSE="true"
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Default to build all if no command specified
if [[ -z "$COMMAND" ]]; then
    BUILD_BACKEND="true"
    BUILD_CERTS="true"
    COMMAND="all"
fi

# Main execution
log_info "=== Updater Server Build Script ==="
log_info "Command: $COMMAND"
log_info "Build type: $BUILD_TYPE"
log_info "Project root: $PROJECT_ROOT"
echo

case $COMMAND in
    all)
        check_dependencies
        if [[ "$BUILD_CERTS" == "true" ]]; then
            generate_certificates
        fi
        if [[ "$BUILD_BACKEND" == "true" ]]; then
            build_backend
        fi
        ;;
    backend)
        check_dependencies
        build_backend
        ;;
    certs)
        check_dependencies
        generate_certificates
        ;;
    clean)
        clean_build
        ;;
    install)
        install_system_deps
        ;;
    test)
        run_tests
        ;;
    package)
        create_package
        ;;
esac

echo
log_success "Build script completed successfully!"

# Show next steps
case $COMMAND in
    all|backend)
        echo
        log_info "=== Next Steps ==="
        log_info "1. Start the integrated application:"
        log_info "   ./run.sh"
        echo
        log_info "2. Or start server only:"
        log_info "   ./server.sh"
        echo
        log_info "3. Or with HTTPS:"
        log_info "   export UPDATER_ENABLE_HTTPS=true"
        log_info "   ./server.sh"
        ;;
    certs)
        echo
        log_info "=== Next Steps ==="
        log_info "Enable HTTPS and start application:"
        log_info "   export UPDATER_ENABLE_HTTPS=true"
        log_info "   ./run.sh"
        ;;
esac