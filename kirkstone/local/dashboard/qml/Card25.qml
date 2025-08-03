import QtQuick 2.15
import RaucSystem 1.0

DashboardCardBase {
    title: "Boot Info"
    property RaucSystemManager raucSystemManager: null
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10
        
        CardInfoRow {
            label: "Boot Order"
            value: raucSystemManager ? raucSystemManager.bootOrder : "Loading..."
            valueColor: "#ffff44"
            labelWidth: 80
        }
        
        CardInfoRow {
            label: "Booted"
            value: raucSystemManager ? ("Slot " + raucSystemManager.currentBootSlot + " (" + (raucSystemManager.currentBootSlot === "A" ? "/dev/sda2" : "/dev/sda3") + ")") : "Loading..."
            valueColor: "#44ff44"
            labelWidth: 80
        }
        
        CardInfoRow {
            label: "Status"
            value: {
                if (!raucSystemManager) return "Loading..."
                let slotAStatus = raucSystemManager.slotAStatus
                let slotBStatus = raucSystemManager.slotBStatus
                if (slotAStatus === "good" && slotBStatus === "good") return "Good"
                else if (slotAStatus === "good" || slotBStatus === "good") return "Good"
                else return "Bad"
            }
            valueColor: {
                if (!raucSystemManager) return "#ffffff"
                let slotAStatus = raucSystemManager.slotAStatus
                let slotBStatus = raucSystemManager.slotBStatus
                if (slotAStatus === "good" && slotBStatus === "good") return "#44ff44"
                else if (slotAStatus === "good" || slotBStatus === "good") return "#44ff44"
                else return "#ff4444"
            }
            labelWidth: 80
        }
    }
}