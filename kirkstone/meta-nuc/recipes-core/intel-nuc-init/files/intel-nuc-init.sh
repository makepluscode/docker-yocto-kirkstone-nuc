#!/bin/sh
#
# Init Intel NUC
# 
# Set hostname and enable network services
#
#***********************************************************************

RESULT=0

# Set hostname to serial number
hostnamectl set-hostname $(/usr/sbin/dmidecode -s system-serial-number)

# Enable systemd-networkd for static IP configuration
systemctl enable systemd-networkd
systemctl start systemd-networkd

# Enable systemd-resolved for DNS
systemctl enable systemd-resolved
systemctl start systemd-resolved

systemd-notify --ready
exit $RESULT 