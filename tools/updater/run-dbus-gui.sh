#!/bin/bash
# ARCRO D-Bus GUI launcher script
cd "$(dirname "$0")"
exec uv run arcro-dbus-gui "$@"