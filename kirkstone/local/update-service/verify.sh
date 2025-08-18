#!/usr/bin/env bash
# Verification script for update-service deployment
# Usage: ./verify.sh [TARGET_IP]

set -e
TARGET=${1:-192.168.1.100}

echo "🔍 Verifying update-service deployment on $TARGET..."

# Check if service is running
echo "📋 Checking service status..."
ssh "root@$TARGET" 'systemctl is-active update-service' && echo "✅ Service is active" || echo "❌ Service is not active"

# Check D-Bus service registration
echo "📋 Checking D-Bus service registration..."
ssh "root@$TARGET" 'dbus-send --system --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q freedesktop.UpdateService' && echo "✅ D-Bus service registered" || echo "❌ D-Bus service not found"

# Check if RAUC is available
echo "📋 Checking RAUC availability..."
ssh "root@$TARGET" 'dbus-send --system --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q pengutronix.rauc' && echo "✅ RAUC service available" || echo "❌ RAUC service not found"

# Test basic method call
echo "📋 Testing GetSlotStatus method..."
ssh "root@$TARGET" 'dbus-send --system --print-reply --dest=org.freedesktop.UpdateService /org/freedesktop/UpdateService org.freedesktop.UpdateService.GetSlotStatus 2>/dev/null' && echo "✅ Method call successful" || echo "❌ Method call failed"

# Test property access
echo "📋 Testing Operation property..."
ssh "root@$TARGET" 'dbus-send --system --print-reply --dest=org.freedesktop.UpdateService /org/freedesktop/UpdateService org.freedesktop.DBus.Properties.Get string:"org.freedesktop.UpdateService" string:"Operation" 2>/dev/null' && echo "✅ Property access successful" || echo "❌ Property access failed"

# Check logs for errors
echo "📋 Checking recent logs for errors..."
ERRORS=$(ssh "root@$TARGET" 'journalctl -u update-service --since="5 minutes ago" --no-pager | grep -i error | wc -l')
if [ "$ERRORS" -eq 0 ]; then
    echo "✅ No errors in recent logs"
else
    echo "⚠️  Found $ERRORS error(s) in recent logs"
    echo "📝 Recent error logs:"
    ssh "root@$TARGET" 'journalctl -u update-service --since="5 minutes ago" --no-pager | grep -i error | tail -5'
fi

echo ""
echo "🎯 Verification Summary:"
echo "  Target: $TARGET"
echo "  Service: update-service"
echo "  D-Bus: org.freedesktop.UpdateService"
echo ""
echo "📖 Manual verification commands:"
echo "  systemctl status update-service"
echo "  journalctl -u update-service -f"
echo "  dbus-send --system --print-reply --dest=org.freedesktop.UpdateService /org/freedesktop/UpdateService org.freedesktop.UpdateService.GetSlotStatus"
