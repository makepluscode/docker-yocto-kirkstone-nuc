# Meta-Apps Layer

This layer contains various applications for the Yocto build system.

## Structure

```
meta-apps/
├── conf/
│   └── layer.conf
├── classes/
│   ├── app-common.bbclass
│   └── qt5-app.bbclass
├── recipes-apps/
│   └── system-dashboard/
├── recipes-qt5/
├── recipes-qt6/
└── recipes-gui/
```

## Available Classes

### app-common.bbclass
Common functionality for all applications:
- SystemD service support
- Desktop file installation
- Common dependencies

### qt5-app.bbclass
Qt5 specific functionality:
- Qt5 build system integration
- Qt5 specific dependencies
- Qt5 configuration file handling

## Adding New Applications

### For Qt5 Applications
1. Create a new directory in `recipes-apps/`
2. Create your recipe file (e.g., `myapp_1.0.bb`)
3. Inherit from `qt5-app` class:
   ```bitbake
   inherit qt5-app
   ```

### For Other Applications
1. Create a new directory in `recipes-apps/`
2. Create your recipe file
3. Inherit from `app-common` class:
   ```bitbake
   inherit app-common
   ```

## Examples

See `recipes-apps/system-dashboard/` for a complete Qt5 application example.

## Dependencies

This layer depends on:
- `core` layer
- `qt5-layer` (for Qt5 applications) 