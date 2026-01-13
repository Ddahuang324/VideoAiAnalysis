// OverviewPage.qml - 像素级还原重构版
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../styles" as Styles
import "../components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    signal detailRequested(var data)

    property var latestReport: {
        "title": "UX Usability Test - Session A",
        "date": "Today, 10:23 AM",
        "findings": ["User struggled with the checkout button visibility.", "Navigation flow described as \"intuitive\" by 80% of subjects.", "Critical friction point detected at timestamp 02:14."]
    }

    property var recentHistory: [
        {
            title: "Competitor Analysis - Landing Page",
            date: "Yesterday",
            duration: "05:32"
        },
        {
            title: "Bug Repro - Crash on Load",
            date: "Dec 08",
            duration: "01:15"
        },
        {
            title: "Design Critique V2",
            date: "Dec 05",
            duration: "45:00"
        }
    ]

    ScrollView {
        anchors.fill: parent
        anchors.margins: 40
        clip: true

        ColumnLayout {
            width: root.width - 80
            spacing: 48

            // ========== 1. Header (对标理想稿：左右两极分化) ==========
            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "Overview"
                        color: "#ffffff"
                        font.pixelSize: 42
                        font.weight: Font.Medium
                    }
                    Text {
                        text: "Your recent analysis insights."
                        color: "#71717a"
                        font.pixelSize: 16
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                    spacing: 0
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: "84%"
                        color: "#ffffff"
                        font.pixelSize: 42
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: "EFFICIENCY SCORE"
                        color: "#71717a"
                        font.pixelSize: 10
                        font.letterSpacing: 1.5
                        font.weight: Font.Bold
                    }
                }
            }

            // ========== 2. Content Row ==========
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 32

                // --- 左侧：Hero Card ---
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 800
                    Layout.preferredHeight: 620
                    radius: 24
                    color: "#0c0c0e"
                    border.color: "#18181b"
                    clip: true

                    // 背景预览图
                    Rectangle {
                        id: heroImg
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 380
                        color: "#18181b"
                        // 渐变遮罩还原设计稿
                        Rectangle {
                            anchors.fill: parent
                            gradient: Gradient {
                                GradientStop {
                                    position: 0.0
                                    color: "transparent"
                                }
                                GradientStop {
                                    position: 1.0
                                    color: "#0c0c0e"
                                }
                            }
                        }

                        Rectangle {
                            x: 32
                            y: 32
                            width: 90
                            height: 26
                            radius: 13
                            color: "#3f3f46"
                            Text {
                                anchors.centerIn: parent
                                text: "Latest Analysis"
                                color: "white"
                                font.pixelSize: 10
                                font.weight: Font.Bold
                            }
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.margins: 32
                            text: latestReport.title
                            color: "white"
                            font.pixelSize: 32
                            font.weight: Font.Bold
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 32
                            text: "↗"
                            color: "#71717a"
                            font.pixelSize: 32
                        }
                    }

                    // Card Footer (Key Findings)
                    Rectangle {
                        anchors.top: heroImg.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 32
                        radius: 16
                        color: "#09090b"
                        border.color: "#18181b"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 24
                            spacing: 16
                            Text {
                                text: "Key Findings:"
                                color: "white"
                                font.pixelSize: 15
                                font.weight: Font.Bold
                            }
                            ColumnLayout {
                                spacing: 12
                                Layout.fillWidth: true
                                Repeater {
                                    model: latestReport.findings
                                    RowLayout {
                                        spacing: 12
                                        Rectangle {
                                            width: 4
                                            height: 4
                                            radius: 2
                                            color: "#ef4444"
                                            Layout.alignment: Qt.AlignTop
                                            Layout.topMargin: 6
                                        }
                                        Text {
                                            text: modelData
                                            color: "#a1a1aa"
                                            font.pixelSize: 14
                                            Layout.fillWidth: true
                                            wrapMode: Text.WordWrap
                                            lineHeight: 1.2
                                        }
                                    }
                                }
                            }
                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.detailRequested({
                            title: latestReport.title,
                            date: latestReport.date,
                            duration: "12:04",
                            status: "completed"
                        })
                    }
                }

                // --- 右侧：Recent History (横向扁平化样式) ---
                ColumnLayout {
                    Layout.preferredWidth: 420
                    Layout.alignment: Qt.AlignTop
                    spacing: 24

                    Text {
                        text: "RECENT HISTORY"
                        color: "#71717a"
                        font.pixelSize: 10
                        font.letterSpacing: 1.5
                        font.weight: Font.Bold
                    }

                    ColumnLayout {
                        spacing: 12
                        Layout.fillWidth: true
                        Repeater {
                            model: recentHistory
                            Rectangle {
                                Layout.fillWidth: true
                                height: 100
                                radius: 16
                                color: "#0c0c0e"
                                border.color: "#18181b"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 16

                                    Rectangle {
                                        width: 68
                                        height: 68
                                        radius: 8
                                        color: "#18181b" // 缩略图占位
                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 20
                                            height: 20
                                            radius: 4
                                            color: "#27272a"
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4
                                        Text {
                                            text: modelData.title
                                            color: "#ffffff"
                                            font.pixelSize: 15
                                            font.weight: Font.Medium
                                            elide: Text.ElideRight
                                        }
                                        RowLayout {
                                            spacing: 8
                                            Text {
                                                text: "🕒 " + modelData.duration
                                                color: "#71717a"
                                                font.pixelSize: 12
                                            }
                                            Text {
                                                text: "•"
                                                color: "#3f3f46"
                                            }
                                            Text {
                                                text: modelData.date
                                                color: "#71717a"
                                                font.pixelSize: 12
                                            }
                                        }
                                    }

                                    Text {
                                        text: "•••"
                                        color: "#3f3f46"
                                        Layout.alignment: Qt.AlignTop
                                        font.pixelSize: 14
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onEntered: parent.border.color = "#3f3f46"
                                    onExited: parent.border.color = "#18181b"
                                    onClicked: root.detailRequested({
                                        title: modelData.title,
                                        date: modelData.date,
                                        duration: modelData.duration,
                                        status: "completed"
                                    })
                                }
                            }
                        }
                    }
                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
