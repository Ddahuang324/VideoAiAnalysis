// WindowButton.qml - 窗口控制按钮组件
// 用于最小化、最大化、关闭按钮

import QtQuick 2.15
import "../styles" as Styles

Rectangle {
    id: root
    width: 46
    height: parent.height
    color: mouseArea.containsMouse ? hoverColor : "transparent"

    // ==================== 公共 API ====================

    property string icon: ""  // "minimize" | "maximize" | "restore" | "close"
    property color hoverColor: Styles.ThemeManager.sidebarItemHover
    property color iconColor: Styles.ThemeManager.textSecondary
    property color iconHoverColor: Styles.ThemeManager.textPrimary

    signal clicked

    // ==================== 动画 ====================

    Behavior on color {
        ColorAnimation {
            duration: Styles.ThemeManager.animFast
        }
    }

    // ==================== 图标 ====================

    Item {
        anchors.centerIn: parent
        width: 12
        height: 12

        // 最小化图标 - 横线
        Rectangle {
            visible: root.icon === "minimize"
            anchors.centerIn: parent
            width: 10
            height: 1
            color: mouseArea.containsMouse ? root.iconHoverColor : root.iconColor

            Behavior on color {
                ColorAnimation {
                    duration: Styles.ThemeManager.animFast
                }
            }
        }

        // 最大化图标 - 空心方框
        Rectangle {
            visible: root.icon === "maximize"
            anchors.centerIn: parent
            width: 10
            height: 10
            color: "transparent"
            border.width: 1
            border.color: mouseArea.containsMouse ? root.iconHoverColor : root.iconColor

            Behavior on border.color {
                ColorAnimation {
                    duration: Styles.ThemeManager.animFast
                }
            }
        }

        // 还原图标 - 两个重叠方框
        Item {
            visible: root.icon === "restore"
            anchors.centerIn: parent
            width: 10
            height: 10

            Rectangle {
                x: 2
                y: 0
                width: 8
                height: 8
                color: "transparent"
                border.width: 1
                border.color: mouseArea.containsMouse ? root.iconHoverColor : root.iconColor
            }

            Rectangle {
                x: 0
                y: 2
                width: 8
                height: 8
                color: Styles.ThemeManager.titleBarBg
                border.width: 1
                border.color: mouseArea.containsMouse ? root.iconHoverColor : root.iconColor
            }
        }

        // 关闭图标 - X
        Item {
            visible: root.icon === "close"
            anchors.centerIn: parent
            width: 10
            height: 10

            Rectangle {
                anchors.centerIn: parent
                width: 12
                height: 1
                rotation: 45
                color: mouseArea.containsMouse ? root.iconHoverColor : root.iconColor

                Behavior on color {
                    ColorAnimation {
                        duration: Styles.ThemeManager.animFast
                    }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 12
                height: 1
                rotation: -45
                color: mouseArea.containsMouse ? root.iconHoverColor : root.iconColor

                Behavior on color {
                    ColorAnimation {
                        duration: Styles.ThemeManager.animFast
                    }
                }
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
