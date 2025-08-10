require rauc.inc
require rauc-1.13.inc

inherit native

# Remove target-specific configurations for native build
PACKAGECONFIG ??= "service network gpt"

# Native build doesn't need systemd services or target files
do_install () {
    cmake_do_install
}