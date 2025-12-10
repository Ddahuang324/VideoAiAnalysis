// ArchivePage.qml - 历史记录页面
// 显示分析历史列表

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Rectangle {
    id: root
    color: Styles.ThemeManager.bgPrimary

    // 模拟历史记录数据
    ListModel {
        id: historyModel

        ListElement {
            title: "项目演示录制"
            date: "2025-12-10 14:30"
            duration: "05:23"
            status: "completed"
        }
        ListElement {
            title: "代码 Review 会议"
            date: "2025-12-09 10:15"
            duration: "12:45"
            status: "completed"
        }
        ListElement {
            title: "产品设计讨论"
            date: "2025-12-08 16:00"
            duration: "08:12"
            status: "analyzing"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Styles.ThemeManager.spacingLg
        spacing: Styles.ThemeManager.spacingMd

        // 标题行
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "📁 历史记录"
                color: Styles.ThemeManager.textPrimary
                font.pixelSize: Styles.ThemeManager.fontSizeH2
                font.weight: Font.Bold
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: historyModel.count + " 条记录"
                color: Styles.ThemeManager.textSecondary
                font.pixelSize: Styles.ThemeManager.fontSizeBody
            }
        }

        // 历史记录列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: historyModel
            spacing: Styles.ThemeManager.spacingSm
            clip: true

            delegate: Rectangle {
                width: listView.width
                height: 80
                radius: Styles.ThemeManager.radiusMd
                color: mouseArea.containsMouse ? Styles.ThemeManager.bgSecondary : Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border

                Behavior on color {
                    ColorAnimation {
                        duration: Styles.ThemeManager.animFast
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Styles.ThemeManager.spacingMd
                    spacing: Styles.ThemeManager.spacingMd

                    // 缩略图占位
                    Rectangle {
                        width: 100
                        height: 56
                        radius: Styles.ThemeManager.radiusSm
                        color: Styles.ThemeManager.bgTertiary

                        Text {
                            anchors.centerIn: parent
                            text: "🎬"
                            font.pixelSize: 24
                        }
                    }

                    // 信息列
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Styles.ThemeManager.spacingXs

                        Text {
                            text: model.title
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeBody
                            font.weight: Font.Medium
                        }

                        Text {
                            text: model.date + " · " + model.duration
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: Styles.ThemeManager.fontSizeSmall
                        }
                    }

                    // 状态标签
                    Rectangle {
                        width: statusText.width + 16
                        height: 24
                        radius: Styles.ThemeManager.radiusSm
                        color: model.status === "completed" ? Styles.ThemeManager.success + "20" : Styles.ThemeManager.warning + "20"

                        Text {
                            id: statusText
                            anchors.centerIn: parent
                            text: model.status === "completed" ? "已完成" : "分析中"
                            color: model.status === "completed" ? Styles.ThemeManager.success : Styles.ThemeManager.warning
                            font.pixelSize: Styles.ThemeManager.fontSizeSmall
                        }
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("Open record:", model.title)
                }
            }
        }

        // 空状态提示
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: historyModel.count === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Styles.ThemeManager.spacingMd

                Text {
                    text: "📭"
                    font.pixelSize: 60
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "暂无历史记录"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }
}
