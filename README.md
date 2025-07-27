# yocto-nuc-docker

Docker-based Yocto build environment for Intel NUC using the Kirkstone (Yocto 4.0 LTS) release.

## 🐳 Usage

### 1. Quick Start

```bash
./download.sh          # Download Yocto layers (first time)
./run-docker.sh        # Enter Docker container
bitbake nuc-image-qt5  # Build Qt5 image
```

For detailed build instructions, see [doc/00-build-guide.md](doc/00-build-guide.md).

## 📚 Documentation

The project documentation is organized with numbered prefixes for easy navigation:

### Build Documentation (00-09)
- **[00-build-guide.md](doc/00-build-guide.md)** - Complete build process guide for Intel NUC Yocto images

### RAUC Documentation (31-32)
- **[31-rauc-grub-good-mark.md](doc/31-rauc-grub-good-mark.md)** - RAUC + GRUB good-mark flow and boot management
- **[32-rauc-service-dbus-api.md](doc/32-rauc-service-dbus-api.md)** - RAUC Installer D-Bus API reference

### Qt Documentation (41-49)
- **[41-qt-graphics-pipeline.md](doc/41-qt-graphics-pipeline.md)** - Qt graphics pipeline guide for Intel NUC

## 🧹 Clean Docker Environment

`clean.sh` removes the Yocto build container and image if they exist:

```bash
./clean.sh
```

- Removes the running or stopped container named `yocto-nuc`.
- Removes the Docker image `yocto-nuc:builder` if present.

---

## 📥 Download Yocto Layers

`download.sh` clones all required Yocto layers into the `kirkstone` directory:

```bash
./download.sh
```

- Clones poky, meta-openembedded, meta-intel, and meta-rauc (kirkstone branch).
- Skips cloning if the directory already exists.

---

## 💾 Flash Image to USB

`flash.sh` writes a `.wic` image to a selected USB device using `dd`:

```bash
./flash.sh
```

- Automatically detects `.wic` files in the current directory, or prompts for the path.
- Lists available removable devices using `lsblk`.
- Prompts for the device name (e.g., `sdb`).
- Asks for confirmation before erasing and flashing.
- Unmounts all partitions on the selected device.
- Uses `dd` to write the image and shows progress.
- Handles errors and aborts safely if any step fails.

---