// HomePage.qml - 首页
// 显示应用概览和快速操作入口

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles
import "../components" as Components

Rectangle {
    id: root
    anchors.fill: parent
    color: Styles.ThemeManager.bgPrimary

    // ==================== 内容布局 ====================
    ColumnLayout {
        anchors.centerIn: parent
        spacing: Styles.ThemeManager.spacingLg

        // 欢迎标题 (Minimalist)
        Text {
            text: "VideoSys"
            color: Styles.ThemeManager.textPrimary
            font.pixelSize: 32
            font.weight: Font.ExtraBold
            font.family: Styles.ThemeManager.fontFamily
            font.letterSpacing: -1
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Minimalist Video Analysis"
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: Styles.ThemeManager.fontSizeBody
            font.family: Styles.ThemeManager.fontFamily
            Layout.alignment: Qt.AlignHCenter
        }

        // 快速操作区域
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Styles.ThemeManager.spacingMd
            Layout.topMargin: Styles.ThemeManager.spacingXl

            // 开始录制卡片
            Components.GlassButton {
                Layout.preferredWidth: 220
                Layout.preferredHeight: 140

                // Content Override? No, GlassButton logic is inside.
                // Using Rectangle for custom card instead of abusing GlassButton if it's meant for simple clicks.
            }
            // Actually, let's keep the Rectangle approach but style it properly.

            // Card 1: Record
            Rectangle {
                width: 220
                height: 140
                radius: Styles.ThemeManager.radiusLg
                color: mouseArea1.containsMouse ? Styles.ThemeManager.surfaceHover : Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12

                    // Icon (Red Dot)
                    Rectangle {
                        width: 24
                        height: 24
                        radius: 12
                        color: Styles.ThemeManager.primary
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Start Recording"
                        color: Styles.ThemeManager.textPrimary
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        font.family: Styles.ThemeManager.fontFamily
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Capture & Analyze"
                        color: Styles.ThemeManager.textMuted
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                MouseArea {
                    id: mouseArea1
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("Navigate to Record page")
                }
            }

            // Card 2: History
            Rectangle {
                width: 220
                height: 140
                radius: Styles.ThemeManager.radiusLg
                color: mouseArea2.containsMouse ? Styles.ThemeManager.surfaceHover : Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12

                    // Icon (Square lines)
                    Rectangle {
                        width: 20
                        height: 24
                        color: "transparent"
                        border.width: 2
                        border.color: Styles.ThemeManager.textSecondary
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "History"
                        color: Styles.ThemeManager.textPrimary
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        font.family: Styles.ThemeManager.fontFamily
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Review Archives"
                        color: Styles.ThemeManager.textMuted
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                MouseArea {
                    id: mouseArea2
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("Navigate to Archive page")
                }
            }
        }

        // 状态信息
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Styles.ThemeManager.spacingXl
            width: 400
            height: 48
            radius: Styles.ThemeManager.radiusMd
            color: "transparent" // Minimalist: no background if possible, or subtle
            border.width: 1
            border.color: Styles.ThemeManager.borderLight

            RowLayout {
                anchors.centerIn: parent
                spacing: Styles.ThemeManager.spacingMd

                Text {
                    text: "i" // Info icon
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: Styles.ThemeManager.primary

                    Rectangle {
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        radius: 9
                        color: "transparent"
                        border.width: 1
                        border.color: Styles.ThemeManager.primary
                    }
                }

                Text {
                    text: "Quick Start: Press Ctrl+R"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                    font.family: Styles.ThemeManager.fontFamily
                }
            }
        }

        // ==================== 测试 C++ 调用按钮 (Minimalist) ====================
        Components.GlassButton {
            id: testButton
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Styles.ThemeManager.spacingLg
            Layout.preferredWidth: 200
            Layout.preferredHeight: 40
            text: "System Diagnostic" // More professional name

            onClicked: {
                console.log("点击测试按钮，调用 C++ 模块...");
                var result = videoViewModel.testCppCall();
                console.log("C++ 调用结果: " + result);
            }
        }

        // 显示测试结果
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Styles.ThemeManager.spacingSm
            text: videoViewModel.result
            color: Styles.ThemeManager.textMuted
            font.pixelSize: Styles.ThemeManager.fontSizeSmall
            font.family: Styles.ThemeManager.fontFamily
            visible: videoViewModel.result !== ""
        }
    }
}
