# RAUC + GRUB Good-Mark Flow

This document explains the process where **RAUC** and **GRUB** cooperate to record and manage 'successful boot (good-mark)' based on Intel NUC (x86-64 UEFI) images.

## Overview

RAUC selects one of two root partitions (`rootfs.0`, `rootfs.1`) in an *A/B* configuration for booting. After successful boot completion, the current slot is marked as *good*. **GRUB** is used as the bootloader, and slot states and boot order are managed through environment variables in the **grubenv partition** (e.g., `/dev/sda1`) rather than the ESP.

## Required Components

- **ESP** (`/boot/EFI/BOOT/grub.cfg`): GRUB config containing slot logic and `rauc.slot=` command line
- **grubenv** (`/grubenv/grubenv`): Storage for variables like `ORDER`, `A_OK`, `B_OK`, `A_TRY`, `B_TRY`
- **systemd service** `rauc-mark-good.service`: Executes `rauc mark-good` upon successful boot

## Boot and Good-Mark Sequence

The following sequence diagram shows the first boot process after a normal update as an example.

```mermaid
autonumber
sequenceDiagram
    participant FW as UEFI/BIOS
    participant GRUB as GRUB EFI Loader
    participant ENV as grubenv (hd0,1)
    participant CFG as grub.cfg (ESP)
    participant KERN as Linux Kernel
    participant SYSTD as systemd
    participant RMG as rauc-mark-good.service
    participant RAUC

    FW->>GRUB: Load BOOTX64.EFI
    GRUB->>ENV: load_env grubenv
    GRUB->>CFG: Parse slot logic
    GRUB->>GRUB: Choose slot **A/B**
    GRUB->>KERN: Boot kernel + `rauc.slot=<X>`
    KERN->>SYSTD: Userspace hand-off
    SYSTD->>RMG: Start (condition: `rauc.slot` present)
    RMG->>RAUC: `rauc mark-good`
    RAUC->>ENV: `fw_setenv <X>_OK=1`, `<X>_TRY=0`
    RAUC-->>RMG: Success
    RMG-->>SYSTD: Exit 0
    note over RAUC,ENV: grubenv updated → Next boot recognizes <X> slot as *good*
```

## Detailed Steps

1. **UEFI firmware** loads `BOOTX64.EFI` from ESP, starting **GRUB**.
2. GRUB reads boot variables (ORDER, A_OK...) from `(hd0,1)/grubenv` file.
3. The script in `grub.cfg`:
   - Scans slots in `ORDER` sequence
   - Selects the first slot where `*_OK == 1` and `*_TRY == 0` as the default boot entry
4. The selected menu entry boots the kernel with `rauc.slot=<A|B>` command line parameter.
5. When Linux userspace starts, **systemd** satisfies the `ConditionKernelCommandLine=|rauc.slot` condition and executes `rauc-mark-good.service`.
6. The service calls `rauc mark-good` command, sending a "current slot boot success" message to the RAUC daemon (DBus).
7. RAUC records `<SLOT>_OK=1` and `<SLOT>_TRY=0` in grubenv using `fw_setenv` (libubootenv), confirming the slot as *good*.
8. On subsequent reboots, GRUB recognizes the same slot as *good*, and in case of update failure, it selects the rollback slot based on TRY flags.

## Troubleshooting Checklist

- If `/proc/cmdline` does not contain the `rauc.slot=` parameter, **rauc-mark-good** will not execute.
- Use `grub-editenv /grubenv/grubenv list` command to verify that variables like `ORDER` exist.
- If `rauc status` remains *bad*, the `*_OK` variable was not set in grubenv or write operation failed.

## References

- RAUC Documentation → Bootloader Integration → GRUB
- Yocto meta-rauc layer `classes/bundle.bbclass`
- U-Boot fw_setenv / libubootenv documentation 