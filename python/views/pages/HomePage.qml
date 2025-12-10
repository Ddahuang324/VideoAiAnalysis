// HomePage.qml - 首页
// 显示应用概览和快速操作入口

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Rectangle {
    id: root
    color: Styles.ThemeManager.bgPrimary

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Styles.ThemeManager.spacingLg

        // 欢迎标题
        Text {
            text: "🎬 AI Video Analysis System"
            color: Styles.ThemeManager.textPrimary
            font.pixelSize: Styles.ThemeManager.fontSizeH1
            font.weight: Font.Bold
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "欢迎回来！选择左侧菜单开始使用"
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: Styles.ThemeManager.fontSizeBody
            Layout.alignment: Qt.AlignHCenter
        }

        // 快速操作区域
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Styles.ThemeManager.spacingMd
            Layout.topMargin: Styles.ThemeManager.spacingXl

            // 开始录制卡片
            Rectangle {
                width: 200
                height: 150
                radius: Styles.ThemeManager.radiusLg
                color: Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Styles.ThemeManager.spacingSm

                    Text {
                        text: "🎥"
                        font.pixelSize: 40
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "开始录制"
                        color: Styles.ThemeManager.textPrimary
                        font.pixelSize: Styles.ThemeManager.fontSizeH3
                        font.weight: Font.Medium
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "录制屏幕并分析"
                        color: Styles.ThemeManager.textSecondary
                        font.pixelSize: Styles.ThemeManager.fontSizeSmall
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("Navigate to Record page")
                }
            }

            // 历史记录卡片
            Rectangle {
                width: 200
                height: 150
                radius: Styles.ThemeManager.radiusLg
                color: Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Styles.ThemeManager.spacingSm

                    Text {
                        text: "📁"
                        font.pixelSize: 40
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "历史记录"
                        color: Styles.ThemeManager.textPrimary
                        font.pixelSize: Styles.ThemeManager.fontSizeH3
                        font.weight: Font.Medium
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "查看分析历史"
                        color: Styles.ThemeManager.textSecondary
                        font.pixelSize: Styles.ThemeManager.fontSizeSmall
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                MouseArea {
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
            height: 60
            radius: Styles.ThemeManager.radiusMd
            color: Styles.ThemeManager.bgSecondary

            RowLayout {
                anchors.centerIn: parent
                spacing: Styles.ThemeManager.spacingMd

                Text {
                    text: "💡"
                    font.pixelSize: 20
                }

                Text {
                    text: "提示：使用快捷键 Ctrl+R 快速开始录制"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                }
            }
        }
    }
}
