// GlassButton.qml - 毛玻璃效果按钮组件
// 半透明背景 + 边框高光效果

import QtQuick 2.15
import "../styles" as Styles

Item {
    id: root
    width: 160
    height: 44

    // ==================== 公共 API ====================

    property string text: "Button"
    property bool enabled: true
    property color accentColor: Styles.ThemeManager.primary

    signal clicked

    // ==================== 背景层 ====================

    Rectangle {
        id: background
        anchors.fill: parent
        radius: Styles.ThemeManager.radiusMd
        color: {
            if (!root.enabled)
                return Styles.ThemeManager.bgTertiary;
            if (mouseArea.pressed)
                return accentColor + "40";
            if (mouseArea.containsMouse)
                return accentColor + "30";
            return Styles.ThemeManager.isDark ? "#30ffffff" : "#30000000";
        }
        border.width: 1
        border.color: {
            if (!root.enabled)
                return Styles.ThemeManager.border;
            if (mouseArea.containsMouse)
                return accentColor;
            return Styles.ThemeManager.isDark ? "#40ffffff" : "#20000000";
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

    // ==================== 光泽层 ====================

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Styles.ThemeManager.radiusMd - 1
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Styles.ThemeManager.isDark ? "#15ffffff" : "#10ffffff"
            }
            GradientStop {
                position: 0.5
                color: "#00ffffff"
            }
        }
    }

    // ==================== 文本 ====================

    Text {
        anchors.centerIn: parent
        text: root.text
        color: root.enabled ? Styles.ThemeManager.textPrimary : Styles.ThemeManager.textMuted
        font.pixelSize: Styles.ThemeManager.fontSizeBody
        font.weight: Font.Medium

        Behavior on color {
            ColorAnimation {
                duration: Styles.ThemeManager.animFast
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

    opacity: enabled ? 1.0 : 0.5

    Behavior on opacity {
        NumberAnimation {
            duration: Styles.ThemeManager.animFast
        }
    }
}
