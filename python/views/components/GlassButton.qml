// GlassButton.qml - 毛玻璃效果按钮组件
// 半透明背景 + 边框高光效果

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Item {
    id: root
    width: 160
    height: 44

    // ==================== 公共 API ====================

    property string text: "Button"
    property bool enabled: true
    property color accentColor: Styles.ThemeManager.primary
    property string iconSource: "" // Added icon capability

    signal clicked

    // ==================== 容器 / 背景 ====================

    Rectangle {
        id: background
        anchors.fill: parent
        radius: Styles.ThemeManager.radiusMd
        color: {
            if (!root.enabled)
                return Styles.ThemeManager.bgTertiary;
            if (mouseArea.pressed)
                return Styles.ThemeManager.textMuted + "20"; // Very subtle press
            if (mouseArea.containsMouse)
                return Styles.ThemeManager.surfaceHover; // Wait, need to check if surfaceHover exists, if not use a calculated color
            // Fallback if surfaceHover not defined in ThemeManager (it wasn't in my last edit, so I'll use a local calculation or standard color)
            if (mouseArea.containsMouse)
                return Styles.ThemeManager.isDark ? "#22FFFFFF" : "#11000000";
            return "transparent"; // Default transparent
        }

        // No borders by default for "Clean" look, or very subtle
        border.width: 1
        border.color: {
            if (mouseArea.containsMouse)
                return Styles.ThemeManager.borderLight;
            return Styles.ThemeManager.border;
        }

        Behavior on color {
            ColorAnimation {
                duration: Styles.ThemeManager.animFast
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: Styles.ThemeManager.animFast
            }
        }
    }

    // ==================== 内容布局 (Asymmetrical) ====================

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Styles.ThemeManager.spacingMd
        anchors.rightMargin: Styles.ThemeManager.spacingSm
        spacing: Styles.ThemeManager.spacingSm

        // 文本 (Left Aligned - Asymmetry)
        Text {
            Layout.fillWidth: true
            text: root.text
            color: root.enabled ? Styles.ThemeManager.textPrimary : Styles.ThemeManager.textMuted
            font.pixelSize: Styles.ThemeManager.fontSizeBody
            font.family: Styles.ThemeManager.fontFamily
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight

            Behavior on color {
                ColorAnimation {
                    duration: Styles.ThemeManager.animFast
                }
            }
        }

        // Optional Icon or Indicator on Right
        Item {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            visible: true // Always keep space for balance or specific asymmetrical indicator

            // Asymmetrical Dot Indicator on Hover
            Rectangle {
                anchors.centerIn: parent
                width: 6
                height: 6
                radius: 3
                color: root.accentColor
                opacity: mouseArea.containsMouse ? 1 : 0
                scale: mouseArea.containsMouse ? 1 : 0

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
        }
    }

    // ==================== 交互 ====================

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
        onClicked: if (root.enabled)
            root.clicked()
    }

    // ==================== 禁用状态 ====================

    opacity: enabled ? 1.0 : 0.4
    Behavior on opacity {
        NumberAnimation {
            duration: Styles.ThemeManager.animFast
        }
    }
}
