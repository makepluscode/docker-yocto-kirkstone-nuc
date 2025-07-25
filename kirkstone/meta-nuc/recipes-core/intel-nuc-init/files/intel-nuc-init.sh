#!/bin/sh
#
# Init Intel NUC
# 
# Set hostname and enable network services
#
#***********************************************************************

RESULT=0

echo "Starting Intel NUC initialization..."

# Set hostname to serial number (with fallback)
SERIAL_NUMBER=$(/usr/sbin/dmidecode -s system-serial-number 2>/dev/null || echo "nuc-unknown")
if [ -n "$SERIAL_NUMBER" ] && [ "$SERIAL_NUMBER" != "nuc-unknown" ]; then
    echo "Setting hostname to: $SERIAL_NUMBER"
    hostnamectl set-hostname "$SERIAL_NUMBER" || echo "Warning: Failed to set hostname"
else
    echo "Warning: Could not determine serial number, using default hostname"
fi

# Enable and start systemd-networkd (non-blocking)
echo "Configuring network services..."
systemctl enable systemd-networkd || echo "Warning: Failed to enable systemd-networkd"
systemctl start systemd-networkd --no-block || echo "Warning: Failed to start systemd-networkd"

# Enable and start systemd-resolved (non-blocking)
systemctl enable systemd-resolved || echo "Warning: Failed to enable systemd-resolved"
systemctl start systemd-resolved --no-block || echo "Warning: Failed to start systemd-resolved"

echo "Intel NUC initialization completed"
systemd-notify --ready
exit $RESULT 