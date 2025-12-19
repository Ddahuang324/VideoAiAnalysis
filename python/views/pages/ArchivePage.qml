// ArchivePage.qml - 历史记录页面
// 显示分析历史列表

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Rectangle {
    id: root
    anchors.fill: parent
    color: Styles.ThemeManager.bgPrimary

    // 模拟历史记录数据
    ListModel {
        id: historyModel

        ListElement {
            title: "Project Demo Recorder"
            date: "2025-12-10 14:30"
            duration: "05:23"
            status: "completed"
        }
        ListElement {
            title: "Code Review Session"
            date: "2025-12-09 10:15"
            duration: "12:45"
            status: "completed"
        }
        ListElement {
            title: "Product Design Discussion"
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
                text: "Archives"
                color: Styles.ThemeManager.textPrimary
                font.pixelSize: Styles.ThemeManager.fontSizeH2
                font.weight: Font.ExtraBold
                font.family: Styles.ThemeManager.fontFamily
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: historyModel.count + " Records"
                color: Styles.ThemeManager.textSecondary
                font.pixelSize: Styles.ThemeManager.fontSizeBody
                font.family: Styles.ThemeManager.fontFamily
            }
        }

        // 历史记录列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: historyModel
            spacing: 8
            clip: true

            delegate: Rectangle {
                width: listView.width
                height: 72
                radius: Styles.ThemeManager.radiusMd
                color: mouseArea.containsMouse ? Styles.ThemeManager.surfaceHover : Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 16

                    // Thumbnail / Icon Placeholder
                    Rectangle {
                        width: 48
                        height: 48
                        radius: 8
                        color: Styles.ThemeManager.bgTertiary

                        Text {
                            anchors.centerIn: parent
                            text: "V"
                            font.family: Styles.ThemeManager.fontFamily
                            font.weight: Font.Bold
                            color: Styles.ThemeManager.textMuted
                        }
                    }

                    // Info Column
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: model.title
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: 14 // Body size
                            font.weight: Font.Bold
                            font.family: Styles.ThemeManager.fontFamily
                        }

                        Text {
                            text: model.date + " · " + model.duration
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 12
                            font.family: Styles.ThemeManager.fontFamily
                        }
                    }

                    // Status Badge
                    Rectangle {
                        width: statusText.width + 16
                        height: 24
                        radius: 12
                        color: model.status === "completed" ? "transparent" : Styles.ThemeManager.warning
                        border.width: model.status === "completed" ? 1 : 0
                        border.color: model.status === "completed" ? Styles.ThemeManager.success : "transparent"

                        // Lower opacity for background fill only? simpler to just use full colors or transparent borders
                        // Minimalist: transparent bg with colored border for finished, filled for active.

                        // Let's refine:
                        // Completed: Success text, no border/bg or subtle.
                        // Analyzing: Warning bg.

                    }

                    Text {
                        id: statusText
                        text: model.status === "completed" ? "Done" : "Processing"
                        color: model.status === "completed" ? Styles.ThemeManager.success : "#111111" // Dark text on warning
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                        font.weight: Font.Medium

                        // Manual positioning next to the badge if badge is used as dot, or inside.
                        // Let's simplify: Just text on the right.
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

                Rectangle {
                    width: 64
                    height: 64
                    radius: 32
                    color: Styles.ThemeManager.bgTertiary
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "No Archives"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    font.family: Styles.ThemeManager.fontFamily
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }
}
