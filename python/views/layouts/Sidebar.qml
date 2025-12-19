// Sidebar.qml - 侧边栏导航组件
// 响应式展开/收起，包含导航菜单项

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles
import "../components" as Components

Rectangle {
    id: root
    width: expanded ? Styles.ThemeManager.sidebarExpandedWidth : Styles.ThemeManager.sidebarCollapsedWidth
    color: Styles.ThemeManager.sidebarBg

    // ==================== 公共 API ====================

    property bool expanded: true
    property int currentIndex: 0

    signal itemClicked(int index)

    // ==================== 动画 ====================

    Behavior on width {
        NumberAnimation {
            duration: Styles.ThemeManager.animNormal
            easing.type: Easing.OutQuad
        }
    }

    // ==================== 右侧边框 ====================

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Styles.ThemeManager.border
    }

    // ==================== 导航数据模型 (Minimalist) ====================

    ListModel {
        id: navigationModel

        ListElement {
            icon: "⊞" // Overview / Dashboard
            text: "Overview"
        }
        ListElement {
            icon: "◉" // Record / Focus
            text: "Record"
        }
        ListElement {
            icon: "▤" // History / Library
            text: "Library"
        }
        ListElement {
            icon: "⚙" // Settings
            text: "Settings"
        }
    }

    // ==================== 内容布局 ====================

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Styles.ThemeManager.spacingSm
        spacing: Styles.ThemeManager.spacingXs

        // Header (Typography Based)
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 64 // Slightly taller header
            Layout.leftMargin: 8

            RowLayout {
                anchors.fill: parent
                spacing: Styles.ThemeManager.spacingSm

                // Abstract Logo Mark
                Rectangle {
                    width: 24
                    height: 24
                    color: Styles.ThemeManager.primary
                    radius: 6 // Squircle

                    Text {
                        anchors.centerIn: parent
                        text: "V"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        font.family: Styles.ThemeManager.fontFamily
                    }
                }

                Text {
                    text: "VideoSys"
                    color: Styles.ThemeManager.textPrimary
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                    font.family: Styles.ThemeManager.fontFamily
                    visible: root.expanded
                    opacity: root.expanded ? 1 : 0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Styles.ThemeManager.animFast
                        }
                    }
                }
            }
        }

        // 菜单项列表
        Repeater {
            model: navigationModel

            Components.SidebarItem {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                icon: model.icon
                text: model.text
                isSelected: index === root.currentIndex
                showText: root.expanded

                onClicked: {
                    root.currentIndex = index;
                    root.itemClicked(index);
                }
            }
        }

        // 弹性空间
        Item {
            Layout.fillHeight: true
        }

        // 底部主题切换 (Refined)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: Styles.ThemeManager.radiusMd
            color: "transparent" // minimalist, no background unless hover

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Styles.ThemeManager.spacingMd

                Text {
                    text: Styles.ThemeManager.isDark ? "☾" : "☼" // Minimalist toggle icon
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: 18
                    Layout.preferredWidth: 24
                }

                Text {
                    text: Styles.ThemeManager.isDark ? "Dark Mode" : "Light Mode"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                    font.weight: Font.Medium
                    font.family: Styles.ThemeManager.fontFamily
                    visible: root.expanded
                    opacity: root.expanded ? 1 : 0
                    Layout.leftMargin: 12

                    Behavior on opacity {
                        NumberAnimation {
                            duration: Styles.ThemeManager.animFast
                        }
                    }
                }
            }

            MouseArea {
                id: themeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Styles.ThemeManager.toggleTheme()
            }
        }
    }
}
