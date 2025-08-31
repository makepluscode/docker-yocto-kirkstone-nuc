# RAUC Installer D-Bus API

This document provides a brief overview of the `de.pengutronix.rauc.Installer` interface exposed by the RAUC daemon via system bus (systemd-DBus).

| Item | Value |
|------|-------|
| Bus Name | `de.pengutronix.rauc.Installer` |
| Object Path | `/de/pengutronix/rauc/Installer` |
| XML Spec Location | `/usr/share/dbus-1/interfaces/de.pengutronix.rauc.Installer.xml` |

## Methods

| Method | IN (Type) | OUT (Type) | Description |
|--------|-----------|------------|-------------|
| **Install** | `source` (s) | – | Start installation by specifying bundle file (.raucb) path |
| **InstallBundle** | `source`(s) `args`(a{sv}) | – | URL support + additional options |
| **Info** | `bundle`(s) | `compatible`(s) `version`(s) | Query bundle metadata |
| **InspectBundle** | `source`(s) `args`(a{sv}) | `info`(a{sv}) | Bundle detailed information (variant map) |
| **Mark** | `state`(s) `slot_identifier`(s) | `slot_name`(s) `message`(s) | Change slot state<br/>state=`good\|bad\|active` |
| **GetSlotStatus** | – | `slot_status_array`(a(sa{sv})) | All slot status array |
| **GetArtifactStatus** | – | `artifacts`(aa{sv}) | Artifact/repository status |
| **GetPrimary** | – | `primary`(s) | Primary slot recognized by bootloader |

## Properties

| Name | Type | Description |
|------|------|-------------|
| Operation | `s` | Current operation: `idle`, `installing`, … |
| LastError | `s` | Last error message |
| Progress | `(isi)` | (percent:int, message:string, depth:int) |
| Compatible | `s` | System compatible string |
| Variant | `s` | System variant |
| BootSlot | `s` | Current booted slot name |

## Signals

| Signal | Parameters | Description |
|---------|------------|-------------|
| Completed | `result`(i) | Result code after installation completion (0=success) |

## Practical Usage Examples

### Get Slot Status
```bash
busctl call --system \
    de.pengutronix.rauc.Installer \
    /de/pengutronix/rauc/Installer \
    de.pengutronix.rauc.Installer GetSlotStatus
```

### Bundle Installation
```bash
busctl call --system \
    de.pengutronix.rauc.Installer \
    /de/pengutronix/rauc/Installer \
    de.pengutronix.rauc.Installer Install \
    s "/data/update_1.0.raucb"
```

### Progress Monitoring
```bash
watch -n1 'busctl get-property --system \
  de.pengutronix.rauc.Installer \
  /de/pengutronix/rauc/Installer \
  de.pengutronix.rauc.Installer Progress'
```

## XML Inspection (Runtime)
```bash
busctl introspect --system \
  de.pengutronix.rauc.Installer \
  /de/pengutronix/rauc/Installer --xml | less
```

## References

- RAUC Official Documentation: https://rauc.readthedocs.io
- DBus Introspection Specification: https://dbus.freedesktop.org/doc/dbus-specification.html 