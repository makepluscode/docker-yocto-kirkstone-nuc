#!/bin/bash
set -e

echo "🚀 Installing ARCRO for current user with desktop integration"

# Check if we're in the right directory
if [ ! -f "pyproject.toml" ] || [ ! -d "src" ]; then
    echo "❌ Please run from the ARCRO project directory"
    exit 1
fi

echo "📦 Installing Python dependencies..."
uv sync

# Create user bin directory if it doesn't exist
mkdir -p ~/.local/bin

echo "🖥️ Creating user launcher scripts..."

# Create launcher scripts for the user
tee ~/.local/bin/arcro-gui > /dev/null << EOF
#!/bin/bash
cd "$(dirname "\${BASH_SOURCE[0]}")/../../../docker-yocto-kirkstone-nuc/tools/updater"
exec uv run python3 -c "import sys; sys.path.insert(0, 'src'); from rauc_updater.gui.main import main; main()" "\$@"
EOF

tee ~/.local/bin/arcro > /dev/null << EOF
#!/bin/bash
cd "$(dirname "\${BASH_SOURCE[0]}")/../../../docker-yocto-kirkstone-nuc/tools/updater" 
exec uv run python3 -c "import sys; sys.path.insert(0, 'src'); from rauc_updater.cli import main; main()" "\$@"
EOF

# Make scripts executable
chmod +x ~/.local/bin/arcro-gui
chmod +x ~/.local/bin/arcro

# Create user applications directory if it doesn't exist
mkdir -p ~/.local/share/applications

echo "🖥️ Creating desktop entry..."

# Create desktop entry for the user
tee ~/.local/share/applications/arcro.desktop > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=ARCRO
GenericName=RAUC Update Tool
Comment=Advanced RAUC Control & Rollout Operations
Exec=$HOME/.local/bin/arcro-gui
Icon=system-software-update
Terminal=false
Categories=System;Settings;
Keywords=rauc;update;ota;embedded;deployment;
StartupNotify=true
StartupWMClass=ARCRO
EOF

# Make sure ~/.local/bin is in PATH
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    echo "📝 Added ~/.local/bin to PATH in ~/.bashrc"
fi

# Update desktop database for the user
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database ~/.local/share/applications/ 2>/dev/null || true
fi

echo "✅ ARCRO installation complete!"
echo ""
echo "You can now:"
echo "  • Launch from Applications menu (search for 'ARCRO')"
echo "  • Run 'arcro-gui' from terminal (after restarting terminal or running: source ~/.bashrc)"
echo "  • Run 'arcro --help' for CLI usage"
echo ""
echo "The application icon should appear in your system menu."
echo "If you don't see it immediately, try logging out and back in."