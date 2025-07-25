# Yocto Build Report: core-image-minimal-initramfs

**Generated:** 2025. 07. 26. (토) 08:38:56 KST
**Image:** core-image-minimal-initramfs-intel-corei7-64

## 📋 Package Summary

- **Total Packages:** 50
- **Machine:** intel-corei7-64
- **Distro:** nodistro
- **Tune:** corei7-64

## 🏗️ Architecture Distribution

| Architecture | Package Count |
|--------------|---------------|
| corei7-64            |  42 |
| all                  |   6 |
| intel_corei7_64      |   2 |

## 📊 Package Categories (Top 20)

| Category | Package Count |
|----------|---------------|
| initramfs            |   6 |
| util                 |   3 |
| shadow               |   3 |
| grub                 |   3 |
| busybox              |   3 |
| udev                 |   2 |
| linux                |   2 |
| base                 |   2 |
| update               |   1 |
| parted               |   1 |
| ncurses              |   1 |
| libzstd1             |   1 |
| libz1                |   1 |
| libuuid1             |   1 |
| libtinfo5            |   1 |
| libsystemd           |   1 |
| libsmartcols1        |   1 |
| libseccomp           |   1 |
| libreadline8         |   1 |
| libmount1            |   1 |

## 📦 Complete Package List

| Package Name | Architecture | Version |
|--------------|--------------|---------|
| base-files                     | intel_corei7_64 | 3.0.14-r89           |
| base-passwd                    | corei7-64       | 3.5.52-r0            |
| busybox                        | corei7-64       | 1.35.0-r0            |
| busybox-syslog                 | corei7-64       | 1.35.0-r0            |
| busybox-udhcpc                 | corei7-64       | 1.35.0-r0            |
| dosfstools                     | corei7-64       | 4.2-r0               |
| e2fsprogs-mke2fs               | corei7-64       | 1.46.5-r0            |
| grub                           | corei7-64       | 2.06-r0              |
| grub-common                    | corei7-64       | 2.06-r0              |
| grub-editenv                   | corei7-64       | 2.06-r0              |
| initramfs-framework-base       | all             | 1.0-r4               |
| initramfs-module-install       | corei7-64       | 1.0-r1               |
| initramfs-module-install-efi   | corei7-64       | 1.0-r4               |
| initramfs-module-rootfs        | all             | 1.0-r4               |
| initramfs-module-setup-live    | all             | 1.0-r4               |
| initramfs-module-udev          | all             | 1.0-r4               |
| ldconfig                       | corei7-64       | 2.35-r0              |
| libacl1                        | corei7-64       | 2.3.1-r0             |
| libattr1                       | corei7-64       | 2.5.1-r0             |
| libblkid1                      | corei7-64       | 2.37.4-r0            |
| libc6                          | corei7-64       | 2.35-r0              |
| libcap                         | corei7-64       | 2.66-r0              |
| libcom-err2                    | corei7-64       | 1.46.5-r0            |
| libcrypt2                      | corei7-64       | 4.4.33-r0            |
| libe2p2                        | corei7-64       | 1.46.5-r0            |
| libext2fs2                     | corei7-64       | 1.46.5-r0            |
| libkmod2                       | corei7-64       | 29-r0                |
| liblzma5                       | corei7-64       | 5.2.6-r0             |
| libmount1                      | corei7-64       | 2.37.4-r0            |
| libreadline8                   | corei7-64       | 8.1.2-r0             |
| libseccomp                     | corei7-64       | 2.5.3-r0             |
| libsmartcols1                  | corei7-64       | 2.37.4-r0            |
| libsystemd-shared              | corei7-64       | 1:250.14-r0          |
| libtinfo5                      | corei7-64       | 6.3+20220423-r0      |
| libuuid1                       | corei7-64       | 2.37.4-r0            |
| libz1                          | corei7-64       | 1.2.11-r0            |
| libzstd1                       | corei7-64       | 1.5.2-r0             |
| linux-firmware-i915            | all             | 1:20240909-r0        |
| linux-firmware-i915-license    | all             | 1:20240909-r0        |
| ncurses-terminfo-base          | corei7-64       | 6.3+20220423-r0      |
| parted                         | corei7-64       | 3.4-r0               |
| shadow                         | corei7-64       | 4.11.1-r0            |
| shadow-base                    | corei7-64       | 4.11.1-r0            |
| shadow-securetty               | intel_corei7_64 | 4.6-r3               |
| udev                           | corei7-64       | 1:250.14-r0          |
| udev-extraconf                 | corei7-64       | 1.1-r0               |
| update-alternatives-opkg       | corei7-64       | 0.5.0-r0             |
| util-linux-blkid               | corei7-64       | 2.37.4-r0            |
| util-linux-lsblk               | corei7-64       | 2.37.4-r0            |
| util-linux-sulogin             | corei7-64       | 2.37.4-r0            |

## 💿 Generated Image Files

| File Name | Size |
|-----------|------|
| core-image-minimal-initramfs-intel-corei7-64-20250725141017.ext4 |      60M |

## 📏 Package Sizes (Top 20)

| Package Name | Size (bytes) | Size (KB) | Size (MB) |
|--------------|--------------|-----------|-----------|

## 🔧 Build Information

- **Build Directory:** ./kirkstone/build
- **Images Directory:** ./kirkstone/build/tmp-glibc/deploy/images/intel-corei7-64
- **Report Generated:** 2025. 07. 26. (토) 08:39:07 KST
- **Image Install:**     packagegroup-core-boot     packagegroup-base-extended                   intel-nuc-init     rauc  intel-nuc-init rauc systemd-conf
- **Extra Features:**  debug-tweaks
