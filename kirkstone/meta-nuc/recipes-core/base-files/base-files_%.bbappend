FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Override fstab to use SATA paths instead of NVMe
SRC_URI += "file://fstab" 