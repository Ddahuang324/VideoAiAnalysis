// SettingsPage.qml - 设置页面的彻底重构版本
// 解决高度塌陷和视觉重叠问题

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../styles" as Styles

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    // ========== 主滚动区域 ==========
    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        anchors.margins: 32
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        // 使用 Column 替代 ColumnLayout 作为顶层容器，确保子项高度正确累加
        Column {
            width: settingsScroll.width - 24 // 留出滚动条空间
            spacing: 24

            // ========== 页面标题 ==========
            Column {
                spacing: 8
                bottomPadding: 16
                Text {
                    text: "Settings"
                    color: Styles.ThemeManager.text
                    font.pixelSize: 32
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Configure your recording and analysis preferences"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: 14
                }
            }

            // ========== 分组 1: Recording ==========
            SettingGroup {
                title: "Recording"
                iconType: "record"
                Column {
                    width: parent.width
                    spacing: 20
                    SettingItem {
                        label: "Output Directory"
                        value: "C:/Users/Videos/Recordings"
                        isLink: true
                    }
                    SettingItem {
                        label: "Video Quality"
                        value: "4K High Performance (60fps)"
                    }
                    SettingItem {
                        label: "Encoder"
                        value: "NVIDIA NVENC H.264"
                    }
                    SettingItem {
                        label: "Audio Bitrate"
                        value: "320 kbps"
                    }
                }
            }

            // ========== 分组 2: Analysis & AI ==========
            SettingGroup {
                title: "Analysis & AI"
                iconType: "analysis"
                Column {
                    width: parent.width
                    spacing: 20

                    // Gemini 配置
                    SettingInput {
                        label: "Gemini API Key"
                        placeholder: "Enter your API key here..."
                        isPassword: true
                    }

                    SettingItem {
                        label: "AI Model"
                        value: "Gemini 2.0 Flash"
                    }

                    SettingToggle {
                        label: "Auto-Analyze captured videos"
                        checked: true
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Styles.ThemeManager.border
                        opacity: 0.3
                    }

                    SettingItem {
                        label: "Scene Detection"
                        value: "Enabled (Threshold 85%)"
                    }
                    SettingItem {
                        label: "Object Tracking"
                        value: "YOLOv8 Real-time"
                    }
                }
            }

            // ========== 分组 3: Appearance ==========
            SettingGroup {
                title: "Appearance"
                iconType: "appearance"
                Column {
                    width: parent.width
                    spacing: 20
                    SettingItem {
                        label: "Visual Theme"
                        value: Styles.ThemeManager.isDark ? "Dark Mode" : "Light Mode"
                    }
                    SettingItem {
                        label: "Accent Color"
                        value: "System Emerald"
                    }
                    SettingItem {
                        label: "Blur Effects"
                        value: "GPU Accelerated"
                    }
                }
            }

            // ========== 分组 4: About ==========
            SettingGroup {
                title: "About"
                iconType: "info"
                Column {
                    width: parent.width
                    spacing: 12
                    Text {
                        text: "AI Video Analysis System v2.1.0"
                        color: Styles.ThemeManager.text
                        font.pixelSize: 15
                        font.weight: Font.Medium
                    }
                    Text {
                        text: "Stable Release - Phase 3 Core Architecture"
                        color: Styles.ThemeManager.textMuted
                        font.pixelSize: 13
                    }
                    Text {
                        text: "© 2026 Antigravity Creative. All rights reserved."
                        color: Styles.ThemeManager.textMuted
                        font.pixelSize: 11
                        topPadding: 10
                    }
                }
            }

            // 底部占位
            Item {
                height: 40
                width: 1
            }
        }
    }

    // ========== 内部辅助组件: SettingGroup (卡片容器) ==========
    component SettingGroup: Rectangle {
        id: groupRoot
        property string title: ""
        property string iconType: ""
        default property alias content: innerContent.data

        width: parent.width
        // 高度由内部 layout 自动计算
        height: groupLayout.implicitHeight + 48
        radius: 16
        color: Styles.ThemeManager.bgSurface
        border.width: 1
        border.color: Styles.ThemeManager.border

        ColumnLayout {
            id: groupLayout
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            RowLayout {
                spacing: 12
                Canvas {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.strokeStyle = Styles.ThemeManager.primary;
                        ctx.lineWidth = 1.8;

                        // 绘制图标逻辑
                        if (iconType === "record") {
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, 0, 2 * Math.PI);
                            ctx.stroke();
                            ctx.fillStyle = Styles.ThemeManager.primary;
                            ctx.beginPath();
                            ctx.arc(12, 12, 3, 0, 2 * Math.PI);
                            ctx.fill();
                        } else if (iconType === "analysis") {
                            ctx.strokeRect(4, 4, 6, 6);
                            ctx.strokeRect(14, 4, 6, 6);
                            ctx.strokeRect(4, 14, 6, 6);
                            ctx.strokeRect(14, 14, 6, 6);
                        } else if (iconType === "appearance") {
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, 0, 2 * Math.PI);
                            ctx.stroke();
                            ctx.fillStyle = Styles.ThemeManager.primary;
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, -Math.PI / 2, Math.PI / 2);
                            ctx.fill();
                        } else {
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, 0, 2 * Math.PI);
                            ctx.stroke();
                            ctx.font = "bold 12px serif";
                            ctx.fillStyle = Styles.ThemeManager.primary;
                            ctx.textAlign = "center";
                            ctx.fillText("i", 12, 16);
                        }
                    }
                }
                Text {
                    text: title
                    color: Styles.ThemeManager.text
                    font.pixelSize: 18
                    font.weight: Font.Bold
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Styles.ThemeManager.border
                opacity: 0.5
            }

            // 内容占位容器
            Item {
                id: innerContent
                Layout.fillWidth: true
                // 注意：由于内容是外部注入的 Column，这里需要自适应高度
                implicitHeight: childrenRect.height
            }
        }
    }

    // ========== 内部辅助组件: SettingItem (单行设置) ==========
    component SettingItem: RowLayout {
        property string label: ""
        property string value: ""
        property bool isLink: false

        width: parent.width
        Text {
            text: label
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: 14
            Layout.fillWidth: true
        }
        Text {
            text: value + (isLink ? " ↗" : "")
            color: isLink ? Styles.ThemeManager.primary : Styles.ThemeManager.text
            font.pixelSize: 14
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignRight

            MouseArea {
                anchors.fill: parent
                visible: isLink
                cursorShape: Qt.PointingHandCursor
                onClicked: console.log("Navigate to: " + label)
            }
        }
    }

    // ========== 内部辅助组件: SettingInput (文本输入) ==========
    component SettingInput: ColumnLayout {
        property string label: ""
        property string placeholder: ""
        property bool isPassword: false
        property alias text: inputField.text

        width: parent.width
        spacing: 8

        Text {
            text: label
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: 14
        }

        TextField {
            id: inputField
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            placeholderText: placeholder
            placeholderTextColor: Styles.ThemeManager.textMuted
            echoMode: isPassword ? TextInput.Password : TextInput.Normal
            color: "#ffffff"
            font.pixelSize: 14
            leftPadding: 16
            rightPadding: 16
            verticalAlignment: TextInput.AlignVCenter

            background: Rectangle {
                radius: 8
                color: Styles.ThemeManager.overlayDark
                border.width: 1
                border.color: inputField.activeFocus ? Styles.ThemeManager.primary : Styles.ThemeManager.border
            }
        }
    }

    // ========== 内部辅助组件: SettingToggle (开关) ==========
    component SettingToggle: RowLayout {
        property string label: ""
        property bool checked: false

        width: parent.width
        Text {
            text: label
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: 14
            Layout.fillWidth: true
        }

        Switch {
            id: control
            checked: parent.checked

            indicator: Rectangle {
                implicitWidth: 40
                implicitHeight: 20
                x: control.leftPadding
                y: parent.height / 2 - height / 2
                radius: 10
                color: control.checked ? Styles.ThemeManager.primary : Styles.ThemeManager.overlayDark
                border.color: control.checked ? Styles.ThemeManager.primary : Styles.ThemeManager.border

                Rectangle {
                    x: control.checked ? parent.width - width - 2 : 2
                    y: 2
                    width: 16
                    height: 16
                    radius: 8
                    color: "white"
                    Behavior on x {
                        NumberAnimation {
                            duration: 200
                        }
                    }
                }
            }
        }
    }
}
