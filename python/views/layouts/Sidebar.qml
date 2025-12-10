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

    // ==================== 导航数据模型 ====================

    ListModel {
        id: navigationModel

        ListElement {
            icon: "🏠"
            text: "首页"
        }
        ListElement {
            icon: "🎬"
            text: "录制"
        }
        ListElement {
            icon: "📁"
            text: "历史"
        }
        ListElement {
            icon: "⚙️"
            text: "设置"
        }
    }

    // ==================== 内容布局 ====================

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Styles.ThemeManager.spacingSm
        spacing: Styles.ThemeManager.spacingXs

        // Logo 区域
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Styles.ThemeManager.spacingSm
                spacing: Styles.ThemeManager.spacingSm

                Rectangle {
                    width: 40
                    height: 40
                    radius: Styles.ThemeManager.radiusMd
                    color: Styles.ThemeManager.primary

                    Text {
                        anchors.centerIn: parent
                        text: "🎥"
                        font.pixelSize: 20
                    }
                }

                Text {
                    text: "AI Video"
                    color: Styles.ThemeManager.textPrimary
                    font.pixelSize: Styles.ThemeManager.fontSizeH3
                    font.weight: Font.Bold
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

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Styles.ThemeManager.border
            Layout.topMargin: Styles.ThemeManager.spacingSm
            Layout.bottomMargin: Styles.ThemeManager.spacingSm
        }

        // 菜单项列表
        Repeater {
            model: navigationModel

            Components.SidebarItem {
                Layout.fillWidth: true
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

        // 底部主题切换按钮
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: Styles.ThemeManager.radiusMd
            color: themeMouseArea.containsMouse ? Styles.ThemeManager.sidebarItemHover : "transparent"

            Behavior on color {
                ColorAnimation {
                    duration: Styles.ThemeManager.animFast
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Styles.ThemeManager.spacingMd
                anchors.rightMargin: Styles.ThemeManager.spacingMd
                spacing: Styles.ThemeManager.spacingSm

                Text {
                    text: Styles.ThemeManager.isDark ? "🌙" : "☀️"
                    font.pixelSize: 20
                    Layout.preferredWidth: 28
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: Styles.ThemeManager.isDark ? "暗色主题" : "亮色主题"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    visible: root.expanded
                    opacity: root.expanded ? 1 : 0

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
