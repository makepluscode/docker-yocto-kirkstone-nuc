#!/bin/bash

# Updater Server Build Script
# Builds both Python backend and Qt6 GUI application

set -e  # Exit on any error

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
GUI_DIR="$PROJECT_ROOT/gui"
BUILD_DIR="$GUI_DIR/build"
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
    gui         Build Qt6 GUI only
    certs       Generate SSL certificates
    clean       Clean build artifacts
    install     Install dependencies
    test        Run tests
    package     Create distribution package

Options:
    --debug         Build in debug mode
    --release       Build in release mode (default)
    --verbose       Show verbose output
    --qt-path PATH  Specify Qt6 installation path
    --help          Show this help message

Examples:
    $0                      # Build everything
    $0 backend              # Build backend only
    $0 gui --debug          # Build GUI in debug mode
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
    
    # Check cmake for GUI
    if [[ "$BUILD_GUI" == "true" ]]; then
        if ! command -v cmake &> /dev/null; then
            missing_deps+=("cmake")
        fi
        
        # Check for Qt6
        if [[ -z "$QT_PATH" ]]; then
            if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
                log_warning "Qt6 not found in PATH. You may need to specify --qt-path"
            fi
        fi
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

# Install Qt6 dependencies (platform specific)
install_qt_deps() {
    log_info "Checking Qt6 installation..."
    
    # Try to detect Qt6 automatically
    if [[ -n "$QT_PATH" ]]; then
        export CMAKE_PREFIX_PATH="$QT_PATH:$CMAKE_PREFIX_PATH"
        log_info "Using Qt6 from: $QT_PATH"
    else
        # Try common Qt6 locations
        QT_LOCATIONS=(
            "/usr/lib/x86_64-linux-gnu/cmake/Qt6"
            "/usr/local/Qt/6.*/gcc_64/lib/cmake/Qt6"
            "/opt/Qt/6.*/gcc_64/lib/cmake/Qt6"
            "/usr/lib/cmake/Qt6"
        )
        
        for qt_path in "${QT_LOCATIONS[@]}"; do
            if [[ -d "$qt_path" ]]; then
                export CMAKE_PREFIX_PATH="$qt_path:$CMAKE_PREFIX_PATH"
                log_info "Found Qt6 at: $qt_path"
                break
            fi
        done
    fi
    
    # Provide installation hints if Qt6 not found
    if ! command -v qmake6 &> /dev/null && [[ -z "$CMAKE_PREFIX_PATH" ]]; then
        log_warning "Qt6 not detected. Install with:"
        log_warning "  Ubuntu/Debian: sudo apt install qt6-base-dev qt6-declarative-dev"
        log_warning "  Or download from: https://www.qt.io/download"
        log_warning "  Or specify path: $0 gui --qt-path /path/to/qt6"
    fi
}

# Build Qt6 GUI
build_gui() {
    log_info "Building Qt6 GUI application..."
    
    # Install Qt6 dependencies
    install_qt_deps
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure CMake
    log_info "Configuring CMake..."
    CMAKE_ARGS=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    )
    
    if [[ -n "$QT_PATH" ]]; then
        CMAKE_ARGS+=("-DQt6_DIR=$QT_PATH/lib/cmake/Qt6")
    fi
    
    if [[ "$VERBOSE" == "true" ]]; then
        CMAKE_ARGS+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    cmake "${CMAKE_ARGS[@]}" ..
    
    # Build
    log_info "Building GUI application..."
    
    if command -v nproc &> /dev/null; then
        JOBS=$(nproc)
    else
        JOBS=4
    fi
    
    make -j"$JOBS"
    
    # Verify build
    if [[ -f "UpdaterGUI" ]]; then
        log_success "GUI application built successfully: $BUILD_DIR/UpdaterGUI"
    else
        log_error "GUI build failed - executable not found"
        exit 1
    fi
    
    log_success "GUI build completed"
}

# Clean build artifacts
clean_build() {
    log_info "Cleaning build artifacts..."
    
    # Clean GUI build
    if [[ -d "$BUILD_DIR" ]]; then
        log_info "Removing GUI build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
    
    # Clean Python cache
    log_info "Cleaning Python cache files..."
    find "$PROJECT_ROOT" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
    find "$PROJECT_ROOT" -type f -name "*.pyc" -delete 2>/dev/null || true
    
    # Clean temporary files
    find "$PROJECT_ROOT" -type f -name "*.tmp" -delete 2>/dev/null || true
    find "$PROJECT_ROOT" -type f -name ".DS_Store" -delete 2>/dev/null || true
    
    log_success "Build artifacts cleaned"
}

# Run tests
run_tests() {
    log_info "Running tests..."
    
    cd "$PROJECT_ROOT"
    
    # Test backend
    log_info "Testing backend..."
    if [[ -f "test_client.py" ]]; then
        python3 test_client.py
    else
        log_warning "No backend tests found"
    fi
    
    # Test GUI (if built)
    if [[ -f "$BUILD_DIR/UpdaterGUI" ]]; then
        log_info "Testing GUI application..."
        # Could add GUI tests here
        log_info "GUI tests not implemented yet"
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
    cp generate_certs.sh "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp config.py "$PACKAGE_DIR/$PACKAGE_NAME/"
    cp README.md "$PACKAGE_DIR/$PACKAGE_NAME/"
    
    # Copy GUI if built
    if [[ -f "$BUILD_DIR/UpdaterGUI" ]]; then
        mkdir -p "$PACKAGE_DIR/$PACKAGE_NAME/gui"
        cp "$BUILD_DIR/UpdaterGUI" "$PACKAGE_DIR/$PACKAGE_NAME/gui/"
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
        sudo apt install -y python3 python3-pip cmake build-essential qt6-base-dev qt6-declarative-dev openssl
    elif command -v yum &> /dev/null; then
        log_info "Detected RHEL/CentOS system"
        sudo yum install -y python3 python3-pip cmake gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel openssl
    elif command -v dnf &> /dev/null; then
        log_info "Detected Fedora system"
        sudo dnf install -y python3 python3-pip cmake gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel openssl
    elif command -v brew &> /dev/null; then
        log_info "Detected macOS with Homebrew"
        brew install python3 cmake qt6 openssl
    else
        log_warning "Unknown system - please install dependencies manually"
        log_info "Required: python3, cmake, qt6, openssl"
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
BUILD_GUI="false"
BUILD_CERTS="false"
BUILD_TYPE="Release"
VERBOSE="false"
QT_PATH=""
COMMAND=""

while [[ $# -gt 0 ]]; do
    case $1 in
        all)
            BUILD_BACKEND="true"
            BUILD_GUI="true"
            BUILD_CERTS="true"
            COMMAND="all"
            shift
            ;;
        backend)
            BUILD_BACKEND="true"
            COMMAND="backend"
            shift
            ;;
        gui)
            BUILD_GUI="true"
            COMMAND="gui"
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
        --qt-path)
            QT_PATH="$2"
            shift 2
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
    BUILD_GUI="true"
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
        if [[ "$BUILD_GUI" == "true" ]]; then
            build_gui
        fi
        ;;
    backend)
        check_dependencies
        build_backend
        ;;
    gui)
        check_dependencies
        build_gui
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
        log_info "1. Start the server:"
        log_info "   cd $PROJECT_ROOT"
        log_info "   ./server.sh"
        echo
        log_info "2. Or with HTTPS:"
        log_info "   export UPDATER_ENABLE_HTTPS=true"
        log_info "   ./server.sh"
        if [[ "$BUILD_GUI" == "true" && -f "$BUILD_DIR/UpdaterGUI" ]]; then
            echo
            log_info "3. Run the GUI:"
            log_info "   $BUILD_DIR/UpdaterGUI"
        fi
        ;;
    gui)
        if [[ -f "$BUILD_DIR/UpdaterGUI" ]]; then
            echo
            log_info "=== Next Steps ==="
            log_info "Run the GUI application:"
            log_info "   $BUILD_DIR/UpdaterGUI"
        fi
        ;;
    certs)
        echo
        log_info "=== Next Steps ==="
        log_info "Enable HTTPS and start server:"
        log_info "   export UPDATER_ENABLE_HTTPS=true"
        log_info "   ./server.sh"
        ;;
esac