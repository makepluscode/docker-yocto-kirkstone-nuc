# Dashboard Card Style Guide

## Overview
The dashboard has been refactored to use a consistent 5-row by 6-column grid layout with unified styling for all cards.

## Card Components

### DashboardCardBase
The base component that provides consistent styling for all cards.

**Properties:**
- `title: string` - Card title shown in header
- `isEmpty: bool` - Set to true for empty/placeholder cards
- `emptyText: string` - Text shown in empty cards (default: "Empty")

**Usage:**
```qml
DashboardCardBase {
    title: "My Card"
    isEmpty: false
    
    // Your content here
    Column {
        anchors.centerIn: parent
        // ...
    }
}
```

### CardInfoRow
Helper component for consistent label-value pairs.

**Properties:**
- `label: string` - Label text
- `value: string` - Value text
- `labelWidth: int` - Width of label column (default: 100)
- `labelColor: color` - Label text color (default: "#cccccc")
- `valueColor: color` - Value text color (default: "#ffffff")

**Usage:**
```qml
CardInfoRow {
    label: "CPU Usage"
    value: "45%"
    labelWidth: 90
    valueColor: "#44ff44"
}
```

## Layout Structure

### Grid Layout
- **5 rows × 6 columns** = 30 total cards
- **Spacing:** 8px between cards
- **Card sizing:** All cards automatically fill their grid cell

### Card Positioning
Cards are positioned left-to-right, top-to-bottom:
```
Row 1: Cards 00-05 (System monitoring)
Row 2: Cards 06-11 (System info & boot management)
Row 3: Cards 12-17 (Available for future use)
Row 4: Cards 18-23 (Available for future use)
Row 5: Cards 24-29 (Available for future use)
```

## Color Scheme

### Card Styling
- **Active Card Background:** `#1a1a1a`
- **Active Card Border:** `#444444` (2px)
- **Empty Card Background:** `#0f0f0f`
- **Empty Card Border:** `#222222` (1px)

### Header Colors
- **Active Card Header:** `#2a2a2a`
- **Empty Card Header:** `#1a1a1a`
- **Header Text:** `#ffffff` (active), `#666666` (empty)

### Content Colors
- **Label Text:** `#cccccc`
- **Value Text:** `#ffffff`
- **Success Color:** `#44ff44`
- **Warning Color:** `#ffaa00`
- **Error Color:** `#ff4444`
- **Info Color:** `#ffff44`

## Creating New Cards

### Active Cards
```qml
import QtQuick 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "New Card"
    property SystemInfo systemInfo: null
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        
        CardInfoRow {
            label: "Status"
            value: systemInfo ? systemInfo.someProperty : "N/A"
        }
        
        // Add more rows or custom content
    }
}
```

### Empty Cards
```qml
DashboardCardBase {
    title: "Future Feature"
    isEmpty: true
    emptyText: "Coming Soon"
}
```

## Converting Existing Cards

1. Replace the Rectangle wrapper with `DashboardCardBase`
2. Remove manual styling (color, border, radius)
3. Replace manual title bar with `title` property
4. Use `CardInfoRow` for label-value pairs
5. Center content vertically with `anchors.verticalCenter: parent.verticalCenter`

### Before (Old Style)
```qml
Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: "#1a1a1a"
    border.color: "#444444"
    // ... manual styling
    
    Column {
        // Title bar rectangle
        Rectangle { /* ... */ }
        
        // Content with manual rows
        Row {
            Text { text: "Label:" }
            Text { text: value }
        }
    }
}
```

### After (New Style)
```qml
DashboardCardBase {
    title: "Card Title"
    
    Column {
        anchors.centerIn: parent
        
        CardInfoRow {
            label: "Label"
            value: value
        }
    }
}
```

## Benefits

1. **Consistent Styling:** All cards look the same
2. **Easy Maintenance:** Change styles in one place
3. **Simple Empty Cards:** Just set `isEmpty: true`
4. **Flexible Layout:** 5×6 grid provides more space
5. **Reusable Components:** `CardInfoRow` for consistent data display
6. **Vertical Centering:** All content automatically centered