#!/bin/bash
set -e

echo "🚀 Installing ARCRO on Ubuntu with desktop integration"

# Check if we're in the right directory
if [ ! -f "pyproject.toml" ] || [ ! -d "src" ]; then
    echo "❌ Please run from the ARCRO project directory"
    exit 1
fi

echo "📦 Installing Python dependencies..."
uv sync

echo "📦 Installing ARCRO system-wide..."
# Use full path to uv since sudo doesn't inherit user PATH
sudo $HOME/.cargo/bin/uv pip install -e . --system 2>/dev/null || {
    echo "⚠️  System-wide installation failed, using alternative method..."
    # Alternative: Install using pip3
    sudo pip3 install -e . 2>/dev/null || {
        echo "⚠️  pip3 installation failed, proceeding with user installation..."
    }
}

echo "🖥️ Creating desktop integration..."

# Create desktop entry
sudo tee /usr/share/applications/arcro.desktop > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=ARCRO
GenericName=RAUC Update Tool
Comment=Advanced RAUC Control & Rollout Operations
Exec=/usr/local/bin/arcro-gui
Icon=system-software-update
Terminal=false
Categories=System;Settings;
Keywords=rauc;update;ota;embedded;deployment;
StartupNotify=true
StartupWMClass=ARCRO
EOF

# Create launcher scripts that work with the correct Python path
CURRENT_DIR="$(pwd)"
sudo tee /usr/local/bin/arcro-gui > /dev/null << EOF
#!/bin/bash
cd "$CURRENT_DIR"
exec $HOME/.cargo/bin/uv run python3 -c "import sys; sys.path.insert(0, 'src'); from rauc_updater.gui.main import main; main()" "\$@"
EOF

sudo tee /usr/local/bin/arcro > /dev/null << EOF
#!/bin/bash
cd "$CURRENT_DIR"
exec $HOME/.cargo/bin/uv run python3 -c "import sys; sys.path.insert(0, 'src'); from rauc_updater.cli import main; main()" "\$@"
EOF

# Make scripts executable
sudo chmod +x /usr/local/bin/arcro-gui
sudo chmod +x /usr/local/bin/arcro

# Update desktop database
sudo update-desktop-database

echo "✅ ARCRO installation complete!"
echo ""
echo "You can now:"
echo "  • Launch from Applications menu (search for 'ARCRO')"
echo "  • Run 'arcro-gui' from terminal"
echo "  • Run 'arcro --help' for CLI usage"
echo ""
echo "The application icon should appear in your system menu under System/Settings."