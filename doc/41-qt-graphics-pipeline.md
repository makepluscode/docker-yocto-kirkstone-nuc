# Qt Graphics Pipeline Guide

Detailed guide for Qt application graphics rendering pipeline on Intel NUC.

## Table of Contents

1. [Graphics Pipeline Overview](#graphics-pipeline-overview)
2. [Layer Details](#layer-details)
3. [Platform Comparison](#platform-comparison)
4. [Troubleshooting Guide](#troubleshooting-guide)
5. [Performance Optimization](#performance-optimization)

## Graphics Pipeline Overview

Complete graphics pipeline flow from Qt to GPU:

```
┌─────────────────────────────────┐
│     Qt Application Layer        │  ← QML/QtQuick Application
├─────────────────────────────────┤
│     Qt Scene Graph              │  ← OpenGL/Vulkan Rendering
├─────────────────────────────────┤
│  Qt Platform Abstraction (QPA) │  ← Cross-platform Layer
├─────────────────────────────────┤
│   Qt Platform Plugin Layer     │  ← eglfs/linuxfb/xcb/wayland
├─────────────────────────────────┤
│    Graphics API Layer          │  ← OpenGL ES/EGL/Mesa
├─────────────────────────────────┤
│   Kernel Graphics Stack        │  ← DRM/KMS/Graphics Driver
├─────────────────────────────────┤
│      Hardware Layer            │  ← GPU/Display Controller
└─────────────────────────────────┘
```

## Layer Details

### 1. Qt Application Layer

**Components:**
- QML Engine
- QtQuick Components
- Qt Scene Graph

**Functions:**
- Convert UI elements to Scene Graph nodes
- Animation and state management
- Event handling

**Output:**
- OpenGL/Vulkan rendering commands
- Geometric data (vertices, textures)

### 2. Qt Platform Abstraction (QPA)

**Functions:**
- Provide platform-independent interface
- Window management abstraction
- Input event integration

**Key Classes:**
- `QPlatformIntegration`
- `QPlatformWindow`
- `QPlatformScreen`

### 3. Qt Platform Plugin Layer

#### EGLFS (Embedded Linux FrameBuffer)
```
Qt App → EGL → OpenGL ES → Mesa → DRM/KMS → GPU
```

**Features:**
- Direct GPU hardware acceleration
- Fullscreen only
- Best performance
- Complex configuration

**Configuration File:**
```json
{
  "device": "/dev/dri/card0",
  "hwcursor": false,
  "pbuffers": true,
  "outputs": [
    {
      "name": "HDMI-A-1",
      "mode": "1920x1080@60"
    }
  ]
}
```

#### LinuxFB (Linux FrameBuffer)
```
Qt App → Linux FB → Kernel FB Driver → Display
```

**Features:**
- Software rendering
- Simple configuration
- High stability
- Limited performance

#### XCB (X11)
```
Qt App → X11 → GPU Driver → Display
```

**Features:**
- Window system support
- Multi-application support
- High memory usage

#### Wayland
```
Qt App → Wayland → Compositor → GPU
```

**Features:**
- Modern display server
- Excellent security
- Low latency

### 4. Graphics API Layer

#### Mesa 3D Graphics Library
- Open source OpenGL/Vulkan implementation
- Hardware abstraction layer
- Multiple GPU driver support

#### EGL (Embedded-System Graphics Library)
- Connect OpenGL ES to native platform
- Context management
- Surface management

### 5. Kernel Graphics Stack

#### DRM (Direct Rendering Manager)
- GPU resource management
- Memory management
- Security and synchronization

#### KMS (Kernel Mode Setting)
- Display mode configuration
- Resolution/refresh rate control
- Multi-display support

#### Graphics Driver (Intel i915)
- Hardware-specific command translation
- GPU memory management
- Power management

### 6. Hardware Layer

#### GPU Hardware
- 3D rendering pipeline
- Texture sampling
- Shader execution

#### Display Controller
- Pixel data output
- Display timing control
- Color space conversion

## Data Flow Example

### EGLFS Path (Hardware Acceleration)
```
QML Rectangle {
    color: "blue"
    width: 100
    height: 100
}
     ↓
Qt Scene Graph (Blue Rectangle Node)
     ↓
OpenGL glDrawElements() commands
     ↓
Mesa → Intel i915 driver
     ↓
GPU shader execution
     ↓
Framebuffer → Display
```

### LinuxFB Path (Software Rendering)
```
QML Rectangle
     ↓
Qt Scene Graph
     ↓
Software rasterizer
     ↓
CPU pixel operations
     ↓
/dev/fb0 framebuffer
     ↓
Display
```

## Platform Comparison

| Platform | Performance | Stability | Setup Complexity | Memory Usage | Use Case |
|----------|-------------|-----------|------------------|--------------|----------|
| **EGLFS** | Excellent | Good | High | Low | Embedded, Kiosk |
| **LinuxFB** | Limited | Excellent | Low | Moderate | Simple UI, Debug |
| **XCB** | Good | Good | Low | High | Desktop Environment |
| **Wayland** | Good | Good | Moderate | Moderate | Modern Embedded |

## Troubleshooting Guide

### 1. Qt Platform Plugin Issues

**Symptoms:**
```
qt.qpa.plugin: Could not find the Qt platform plugin "eglfs"
Available platform plugins are: minimal, offscreen, vnc, xcb.
```

**Cause:**
- Missing platform support in qtbase PACKAGECONFIG

**Solution:**
```bitbake
PACKAGECONFIG:pn-qtbase = "eglfs linuxfb kms gbm"
```

### 2. DRM/KMS Access Issues

**Symptoms:**
```
qt.qpa.eglfs.kms: Could not open DRM device
```

**Check:**
```bash
ls -la /dev/dri/
# Check permissions
# Check Intel driver loading
dmesg | grep i915
```

**Solution:**
- Add user to video group
- Verify DRM module loading

### 3. EGL Initialization Failure

**Symptoms:**
```
qt.qpa.eglfs: Failed to initialize EGL display
```

**Check:**
```bash
# Check Mesa installation
find /usr -name "*libEGL*"
# Check GPU information
lspci | grep VGA
```

### 4. Framebuffer Issues

**Symptoms:**
```
qt.qpa.linuxfb: Failed to open framebuffer
```

**Check:**
```bash
ls -la /dev/fb*
cat /proc/fb
```

## Performance Optimization

### EGLFS Optimization

#### 1. KMS Configuration Optimization
```json
{
  "device": "/dev/dri/card0",
  "hwcursor": true,
  "pbuffers": true,
  "outputs": [
    {
      "name": "HDMI-A-1",
      "mode": "1920x1080@60",
      "format": "argb8888",
      "physicalWidth": 400,
      "physicalHeight": 300
    }
  ]
}
```

#### 2. Environment Variable Optimization
```bash
export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
export QT_QPA_EGLFS_KMS_CONFIG=/etc/qt5/eglfs_kms_config.json
export QT_QPA_EGLFS_ALWAYS_SET_MODE=1
```

#### 3. Qt Scene Graph Optimization
```bash
export QSG_RENDER_LOOP=basic
export QSG_RHI_BACKEND=opengl
```

### LinuxFB Optimization

#### 1. Framebuffer Configuration
```bash
# Set color depth
fbset -depth 32
# Set resolution
fbset -xres 1920 -yres 1080
```

#### 2. Qt Rendering Optimization
```bash
export QT_QPA_FB_FORCE_FULLSCREEN=1
export QT_QPA_FB_DISABLE_INPUT=0
```

## Debugging Tools

### Qt Debugging
```bash
# Enable all Qt logging
export QT_LOGGING_RULES="*=true"

# Specific categories only
export QT_LOGGING_RULES="qt.qpa.*=true"
export QT_LOGGING_RULES="qt.qpa.eglfs.*=true"
```

### System Debugging
```bash
# DRM information
cat /sys/kernel/debug/dri/0/i915_display_info

# GPU usage
intel_gpu_top

# Framebuffer information
fbset -i
```

## References

- [Qt Platform Abstraction Documentation](https://doc.qt.io/qt-5/qpa.html)
- [Intel Graphics Driver Documentation](https://01.org/linuxgraphics/documentation)
- [Mesa 3D Documentation](https://docs.mesa3d.org/)
- [DRM/KMS Documentation](https://www.kernel.org/doc/html/latest/gpu/drm-kms.html)

---

*Last updated: 2025-07-26* 