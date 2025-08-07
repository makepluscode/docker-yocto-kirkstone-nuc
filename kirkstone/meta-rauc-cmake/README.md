# meta-rauc-cmake

This Yocto layer provides RAUC (Robust Auto-Update Controller) built with CMake instead of the original Meson build system.

## Overview

This layer replaces the standard `meta-rauc` layer and provides:
- CMake-based build system for RAUC 1.13
- All original functionality preserved (status, install bundles, A/B boot management)
- Cross-compilation support for Intel NUC targets
- D-Bus service integration
- Compatible with existing RAUC configurations

## Features

- **ENABLE_SERVICE**: D-Bus service support ✅
- **ENABLE_CREATE**: Bundle creation capabilities ✅ 
- **ENABLE_NETWORK**: Network update support with libcurl ✅
- **ENABLE_GPT**: GPT partition support with libfdisk ✅
- **ENABLE_JSON**: JSON support (disabled - not available in SDK)
- **ENABLE_STREAMING**: Streaming support (disabled - not available in SDK)

## Usage

1. Add this layer to your `bblayers.conf` instead of `meta-rauc`:
   ```
   /path/to/kirkstone/meta-rauc-cmake \
   ```

2. Build your image as normal:
   ```bash
   bitbake nuc-image-qt5
   ```

3. The resulting image will contain the CMake-built RAUC instead of the meson version.

## Migration from meta-rauc

To migrate from the original meta-rauc layer:

1. Remove `meta-rauc` from `bblayers.conf`
2. Add `meta-rauc-cmake` to `bblayers.conf`
3. Rebuild your image

All existing configurations, certificates, and system.conf files remain compatible.

## Architecture

- **recipes-core/rauc/**: Main RAUC recipes using CMake
- **recipes-core/bundles/**: Bundle creation recipes (unchanged)
- **recipes-core/packagegroups/**: Package group configurations (unchanged)
- **recipes-core/busybox/**: Busybox integration (unchanged)

## Dependencies

- core layer
- openembedded-layer
- Standard dependencies: openssl, glib-2.0, dbus, util-linux, curl

## Compatibility

- Yocto Kirkstone (4.0 LTS)
- Intel NUC (corei7-64) targets
- Compatible with existing RAUC configurations and certificates