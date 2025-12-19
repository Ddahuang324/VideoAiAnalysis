// SidebarItem.qml - 侧边栏菜单项组件
// 支持图标、文本、选中状态和悬停效果

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../styles" as Styles

Rectangle {
    id: root
    width: parent ? parent.width : 200
    height: 48

    // ==================== 公共 API ====================

    property string icon: ""      // 图标文本
    property string text: ""      // 菜单项文本
    property bool isSelected: false
    property bool showText: true  // 控制文本显示/隐藏

    signal clicked

    // ==================== 动画 ====================

    Behavior on color {
        ColorAnimation {
            duration: Styles.ThemeManager.animFast
        }
    }

    // ==================== 主背景 (Asymmetrical Active State) ====================

    // Active state uses a very subtle fill
    color: {
        if (isSelected)
            return Styles.ThemeManager.surfaceHover; // Subtle active background
        if (mouseArea.containsMouse)
            return Styles.ThemeManager.surfaceHover;
        return "transparent";
    }

    // Asymmetrical Radius: Sharp on left if selected? Or just standard soft.
    // Let's keep it clean: Standard radius for now, but maybe left-aligned 0 padding?
    // Actually, let's make it fully rounded for "Design" feel.
    radius: Styles.ThemeManager.radiusMd

    // ==================== 选中/不对称指示器 (Right Side) ====================

    Rectangle {
        id: indicator
        width: 6
        height: 6
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        radius: 3
        color: Styles.ThemeManager.primary
        visible: isSelected
        opacity: isSelected ? 1 : 0
        scale: isSelected ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: Styles.ThemeManager.animFast
            }
        }
        Behavior on scale {
            NumberAnimation {
                duration: Styles.ThemeManager.animFast
                easing.type: Easing.OutBack
            }
        }
    }

    // ==================== 内容布局 ====================

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Styles.ThemeManager.spacingMd
        anchors.rightMargin: Styles.ThemeManager.spacingMd
        spacing: Styles.ThemeManager.spacingSm

        // 图标
        Text {
            text: root.icon
            font.pixelSize: 20
            Layout.preferredWidth: 28
            horizontalAlignment: Text.AlignHCenter
        }

        // 文本
        Text {
            text: root.text
            color: isSelected ? Styles.ThemeManager.textPrimary : Styles.ThemeManager.textSecondary
            font.pixelSize: Styles.ThemeManager.fontSizeBody
            font.weight: isSelected ? Font.Medium : Font.Normal
            Layout.fillWidth: true
            elide: Text.ElideRight

            // 展开/收起时的渐变效果
            opacity: showText ? 1 : 0
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Styles.ThemeManager.animFast
                }
            }

            Behavior on color {
                ColorAnimation {
                    duration: Styles.ThemeManager.animFast
                }
            }
        }
    }

    // ==================== Tooltip (收起模式) ====================

    Rectangle {
        id: tooltip
        visible: !showText && mouseArea.containsMouse
        x: parent.width + 8
        anchors.verticalCenter: parent.verticalCenter
        width: tooltipText.width + 16
        height: tooltipText.height + 8
        radius: Styles.ThemeManager.radiusSm
        color: Styles.ThemeManager.bgCard
        border.width: 1
        border.color: Styles.ThemeManager.border
        z: 1000

        Text {
            id: tooltipText
            anchors.centerIn: parent
            text: root.text
            color: Styles.ThemeManager.textPrimary
            font.pixelSize: Styles.ThemeManager.fontSizeSmall
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
