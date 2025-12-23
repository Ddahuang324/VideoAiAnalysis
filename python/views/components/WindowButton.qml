// WindowButton.qml - 窗口控制按钮组件
// 用于最小化、最大化、关闭按钮

import QtQuick 2.15
import "../styles" as Styles

Rectangle {
    id: root
    width: 46
    height: parent.height

    // ==================== 公共 API ====================

    property string icon: ""  // "minimize" | "maximize" | "restore" | "close"
    property color hoverColor: Styles.ThemeManager.surfaceHover
    property color iconColor: Styles.ThemeManager.textSecondary
    property color iconHoverColor: Styles.ThemeManager.textPrimary

    // Close button specific override
    property bool isCloseButton: icon === "close"

    signal clicked

    // ==================== 背景 ====================

    color: {
        if (mouseArea.containsMouse) {
            return isCloseButton ? Styles.ThemeManager.error : hoverColor;
        }
        return "transparent";
    }

    Behavior on color {
        ColorAnimation {
            duration: Styles.ThemeManager.animFast
        }
    }

    // ==================== 图标 (Geometric) ====================

    Item {
        anchors.centerIn: parent
        width: 10
        height: 10
        opacity: mouseArea.containsMouse ? 1.0 : 0.7

        Behavior on opacity {
            NumberAnimation {
                duration: Styles.ThemeManager.animFast
            }
        }

        // Common Icon Stroke Color
        property color strokeColor: {
            if (mouseArea.containsMouse) {
                return isCloseButton ? "#FFFFFF" : root.iconHoverColor;
            }
            return root.iconColor;
        }

        // Minimize
        Rectangle {
            visible: root.icon === "minimize"
            anchors.centerIn: parent
            width: 10
            height: 1
            color: parent.strokeColor
        }

        // Maximize
        Rectangle {
            visible: root.icon === "maximize"
            anchors.centerIn: parent
            width: 10
            height: 10
            color: "transparent"
            border.width: 1.5 // Slightly thicker for boldness
            border.color: parent.strokeColor
        }

        // Restore
        Item {
            visible: root.icon === "restore"
            anchors.fill: parent

            Rectangle {
                x: 2
                y: 2
                width: 8
                height: 8
                color: "transparent"
                border.width: 1
                border.color: parent.parent.strokeColor
            }
            Rectangle {
                x: 0
                y: 4
                width: 8
                height: 1
                color: parent.parent.strokeColor
                rotation: -45 // Abstract restore? No, stick to standard shapes for window controls to avoid confusion.
                visible: false
            }
            // Standard restore icon is hard with just Rects roughly, let's simplify to a "Square with corner"
            Rectangle {
                x: 1
                y: -1
                width: 8
                height: 8
                color: "transparent"
                border.width: 1
                border.color: parent.parent.strokeColor
                opacity: 0.5
            }
            Rectangle {
                x: -1
                y: 1
                width: 8
                height: 8
                color: "transparent"
                border.width: 1
                border.color: parent.parent.strokeColor
            }
        }

        // Close (X)
        Item {
            visible: root.isCloseButton
            anchors.centerIn: parent
            width: 10
            height: 10

            Rectangle {
                anchors.centerIn: parent
                width: 12
                height: 1.5
                rotation: 45
                color: parent.parent.strokeColor
            }
            Rectangle {
                anchors.centerIn: parent
                width: 12
                height: 1.5
                rotation: -45
                color: parent.parent.strokeColor
            }
        }
    }

    // ==================== 交互 ====================

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
