import QtQuick 2.15
import RaucSystem 1.0
import SystemInfo 1.0

DashboardCardBase {
    title: "Boot Info"
    property RaucSystemManager raucSystemManager: null
    property SystemInfo systemInfo: null

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
            value: raucSystemManager ? raucSystemManager.currentBootSlot : "Loading..."
            valueColor: "#44ff44"
            labelWidth: 80
        }

        CardInfoRow {
            label: "Status"
            value: {
                if (!raucSystemManager) return "Loading..."
                let currentSlot = raucSystemManager.currentBootSlot
                let slotAStatus = raucSystemManager.slotAStatus
                let slotBStatus = raucSystemManager.slotBStatus

                if (currentSlot === "rootfs.0" && slotAStatus === "good") return "Good"
                else if (currentSlot === "rootfs.1" && slotBStatus === "good") return "Good"
                else if (slotAStatus === "good" || slotBStatus === "good") return "Good"
                else return "Bad"
            }
            valueColor: {
                if (!raucSystemManager) return "#ffffff"
                let currentSlot = raucSystemManager.currentBootSlot
                let slotAStatus = raucSystemManager.slotAStatus
                let slotBStatus = raucSystemManager.slotBStatus

                if (currentSlot === "rootfs.0" && slotAStatus === "good") return "#44ff44"
                else if (currentSlot === "rootfs.1" && slotBStatus === "good") return "#44ff44"
                else if (slotAStatus === "good" || slotBStatus === "good") return "#44ff44"
                else return "#ff4444"
            }
            labelWidth: 80
        }

        CardInfoRow {
            label: "RFS"
            value: systemInfo ? systemInfo.rootDevice : "Loading..."
            valueColor: "#44ffff"
            labelWidth: 80
        }
    }
}
