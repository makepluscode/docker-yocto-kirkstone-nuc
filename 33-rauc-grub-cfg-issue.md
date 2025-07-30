# RAUC GRUB Configuration Issue - ACPI Error Resolution

## Problem Description

After implementing A/B partition booting with RAUC, ACPI errors occur during boot:
```
ACPI Error.... (AE_NOT_FOUND)
```

This error was not present before the custom GRUB configuration changes.

## Root Cause Analysis

The ACPI errors are likely caused by:
1. Modified kernel command line parameters in the custom `grub.cfg`
2. Missing or incompatible ACPI-related parameters
3. Changes in boot sequence affecting hardware initialization

## Proposed Solution

### ACPI Error Prevention Parameters

The following kernel command line parameters should be tested to resolve ACPI issues:

```bash
# Basic ACPI disable
acpi=off

# Advanced ACPI disable with interrupt controller settings
acpi=off noapic nolapic

# Alternative: Force ACPI with compatibility mode
acpi=force
```

### Modified GRUB Configuration

The custom `grub.cfg` should include:

```bash
# Kernel command line parameters - ACPI error prevention
CMDLINE="quiet acpi=off noapic"
ROOT_BLOCK_DEVICE_NAME="sda"

# Menu entries for A/B slots
menuentry "Slot A" {
    linux (hd0,2)/boot/bzImage LABEL=boot root=/dev/${ROOT_BLOCK_DEVICE_NAME}p2 rootwait console=ttyS0,115200 console=tty0 $CMDLINE rauc.slot=A
}

menuentry "Slot B" {
    linux (hd0,3)/boot/bzImage LABEL=boot root=/dev/${ROOT_BLOCK_DEVICE_NAME}p3 rootwait console=ttyS0,115200 console=tty0 $CMDLINE rauc.slot=B
}

# Fallback menu entries with additional ACPI workarounds
menuentry "Slot A (ACPI Disabled)" {
    linux (hd0,2)/boot/bzImage LABEL=boot root=/dev/${ROOT_BLOCK_DEVICE_NAME}p2 rootwait console=ttyS0,115200 console=tty0 acpi=off noapic nolapic rauc.slot=A
}

menuentry "Slot B (ACPI Disabled)" {
    linux (hd0,3)/boot/bzImage LABEL=boot root=/dev/${ROOT_BLOCK_DEVICE_NAME}p3 rootwait console=ttyS0,115200 console=tty0 acpi=off noapic nolapic rauc.slot=B
}
```

## Testing Strategy

### Phase 1: Runtime Testing (Recommended Approach)

**Before integrating into Yocto build system, test the configuration at runtime:**

1. **Manual GRUB Configuration Update**
   - Copy the modified `grub.cfg` to the target system
   - Test booting with different ACPI parameters
   - Verify A/B slot switching functionality

2. **Runtime Parameter Testing**
   - Test `acpi=off` parameter
   - Test `acpi=off noapic` combination
   - Test `acpi=off noapic nolapic` combination
   - Test `acpi=force` parameter

3. **Functionality Verification**
   - Confirm A/B slot booting works correctly
   - Verify RAUC slot identification (`rauc.slot=A/B`)
   - Check system stability and performance

### Phase 2: Yocto Integration

**After successful runtime testing:**

1. **Create BitBake Append Files**
   - `grub-bootconf_%.bbappend` for GRUB configuration
   - Update `local.conf.sample` with ACPI parameters

2. **Build and Test**
   - Build new image with integrated changes
   - Flash and verify functionality

## Benefits of Runtime-First Approach

1. **Faster Iteration**: No need to rebuild entire image for each test
2. **Immediate Feedback**: Quick validation of ACPI parameter effectiveness
3. **Risk Reduction**: Identify issues before Yocto integration
4. **Flexibility**: Easy to test multiple parameter combinations

## Next Steps

1. Apply the modified `grub.cfg` to the target system manually
2. Test booting with different ACPI parameter combinations
3. Document which parameters resolve the ACPI errors
4. Integrate successful configuration into Yocto build system
5. Build and deploy final image with confirmed working settings

## Files to Modify

- `kirkstone/meta-nuc/recipes-bsp/grub/files/grub.cfg`
- `kirkstone/meta-nuc/conf/local.conf.sample`
- `kirkstone/meta-nuc/recipes-bsp/grub/grub-bootconf_%.bbappend`

## Testing Commands

```bash
# Manual GRUB config update
scp grub.cfg root@192.168.1.100:/grubenv/EFI/BOOT/grub.cfg

# Verify boot parameters
ssh root@192.168.1.100 "cat /proc/cmdline"

# Test A/B slot switching
ssh root@192.168.1.100 "grub-editenv - set ORDER=B A"
ssh root@192.168.1.100 "reboot"
``` 