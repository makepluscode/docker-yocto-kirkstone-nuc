# Update Service - RAUC D-Bus Broker

## Overview

The Update Service (`update-service`) is a D-Bus broker that provides an abstraction layer between update-agent and RAUC. It exposes the same interface as RAUC but with different method names (prefixed with "Update") and forwards all calls to the actual RAUC service.

## Architecture

```
update-agent  <--D-Bus-->  update-service  <--D-Bus-->  RAUC
     |                           |                        |
     |        New Interface      |     Original Interface |
     +------ UpdateInstall ------+------- Install -------+
     +------ UpdateMark ---------+------- Mark -----------+
     +------ UpdateOperation ----+------- Operation ------+
```

## D-Bus Interface

**Service Name:** `org.freedesktop.UpdateService`
**Object Path:** `/org/freedesktop/UpdateService`
**Interface:** `org.freedesktop.UpdateService`

### Methods

All methods have the same signature as RAUC:

- `Install(path: string)` → forwards to RAUC `Install`
- `InstallBundle(source: string, args: dict)` → forwards to RAUC `InstallBundle`
- `Info(bundle: string)` → forwards to RAUC `Info`
- `Mark(state: string, slot: string)` → forwards to RAUC `Mark`
- `GetSlotStatus()` → forwards to RAUC `GetSlotStatus`
- `GetPrimary()` → forwards to RAUC `GetPrimary`

### Properties

All properties have the same type as RAUC:

- `Operation: string` → forwards to RAUC `Operation`
- `LastError: string` → forwards to RAUC `LastError`
- `Progress: (isi)` → forwards to RAUC `Progress`
- `Compatible: string` → forwards to RAUC `Compatible`
- `BootSlot: string` → forwards to RAUC `BootSlot`

### Signals

- `Completed(result: int)` → forwarded from RAUC `Completed`
- `Progress(percentage: int, message: string, depth: int)` → forwarded from RAUC `Progress`

## Build Instructions

### Cross-compilation for Target

```bash
# Build update-service
./build.sh

# Deploy to target
./deploy.sh [root@TARGET_IP]
```

### Local Development Build

```bash
# Install dependencies
sudo apt-get install libdlt-dev libdbus-1-dev

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

## Usage

### Service Management

```bash
# Start service
systemctl start update-service

# Check status
systemctl status update-service

# View logs
journalctl -u update-service -f

# View DLT logs
dlt-receive -a localhost
```

### Testing

```bash
# Build and run test client
cd build
./test-client

# Test with dbus-send
dbus-send --system --print-reply \
  --dest=org.freedesktop.UpdateService \
  /org/freedesktop/UpdateService \
  org.freedesktop.UpdateService.GetSlotStatus

# Test property access
dbus-send --system --print-reply \
  --dest=org.freedesktop.UpdateService \
  /org/freedesktop/UpdateService \
  org.freedesktop.DBus.Properties.Get \
  string:"org.freedesktop.UpdateService" \
  string:"Operation"
```

## Integration with Update-Agent

To use the broker, update-agent should connect to `org.freedesktop.UpdateService` instead of `de.pengutronix.rauc` and call the same method names.

Example change in update-agent:

```cpp
// Old RAUC direct connection
connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
dbus_message_new_method_call("de.pengutronix.rauc", "/",
                           "de.pengutronix.rauc.Installer", "Install");

// New update-service connection
connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
dbus_message_new_method_call("org.freedesktop.UpdateService",
                           "/org/freedesktop/UpdateService",
                           "org.freedesktop.UpdateService", "Install");
```

## Configuration Files

- **Systemd Service:** `/etc/systemd/system/update-service.service`
- **D-Bus Policy:** `/etc/dbus-1/system.d/org.freedesktop.UpdateService.conf`
- **D-Bus Service:** `/usr/share/dbus-1/system-services/org.freedesktop.UpdateService.service`

## Troubleshooting

### Service Not Starting

```bash
# Check if RAUC is running
systemctl status rauc

# Check D-Bus policy
dbus-send --system --dest=org.freedesktop.DBus \
  /org/freedesktop/DBus org.freedesktop.DBus.ListNames

# Check logs
journalctl -u update-service --no-pager
```

### D-Bus Permission Issues

- Ensure user is in appropriate groups (wheel, nuc)
- Check D-Bus policy file `/etc/dbus-1/system.d/org.freedesktop.UpdateService.conf`
- Restart D-Bus service: `systemctl restart dbus`

### Connection to RAUC Failed

- Verify RAUC service is running: `systemctl status rauc`
- Check RAUC D-Bus interface: `dbus-send --system --dest=de.pengutronix.rauc /`

## Security Considerations

- Service runs as root to access RAUC
- D-Bus policies restrict access to authorized users
- All operations are forwarded to RAUC without modification
- No additional authentication beyond D-Bus policy
