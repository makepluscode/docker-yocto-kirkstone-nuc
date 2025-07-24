FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Override the install script to use sda instead of nvme
SRC_URI += "file://init-install-efi.sh" 