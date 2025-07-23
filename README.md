# yocto-nuc-docker

Docker-based Yocto build environment for Intel NUC using the Kirkstone (Yocto 4.0 LTS) release.

## 🐳 Usage

### 1. Run build environment

```bash
./run-docker.sh
```

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

- Clones poky, meta-openembedded, meta-intel, meta-microservicebus-intel-nuc, and meta-rauc (kirkstone branch).
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