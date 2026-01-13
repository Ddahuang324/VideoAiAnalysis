// DetailPage.qml - 像素级还原重构版 (优化版 V2)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../styles" as Styles
import "../components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#09090b"

    // 属性
    property string recordTitle: "UX Usability Test - Session A"
    property string recordDate: "Today, 10:23 AM"
    property string recordDuration: "12:04"
    property string recordStatus: "completed"

    signal backRequested

    // ==================== 顶部导航栏 ====================
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        color: "#09090b"
        z: 100

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 16

            Item {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Canvas {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.strokeStyle = "#ffffff";
                        ctx.lineWidth = 1.5;
                        ctx.beginPath();
                        ctx.moveTo(9, 2);
                        ctx.lineTo(3, 7);
                        ctx.lineTo(9, 12);
                        ctx.stroke();
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.backRequested()
                }
            }

            ColumnLayout {
                spacing: 0
                Text {
                    text: root.recordTitle
                    color: "#ffffff"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                }
                Text {
                    text: root.recordDate + " • " + root.recordDuration
                    color: "#71717a"
                    font.pixelSize: 11
                }
            }

            Item {
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 16
                Repeater {
                    model: ["download", "share", "more"]
                    Item {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        Canvas {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset();
                                ctx.strokeStyle = "#a1a1aa";
                                ctx.lineWidth = 1.2;
                                ctx.beginPath();
                                if (index === 0) {
                                    ctx.moveTo(8, 2);
                                    ctx.lineTo(8, 12);
                                    ctx.moveTo(4, 8);
                                    ctx.lineTo(8, 12);
                                    ctx.lineTo(12, 8);
                                } else if (index === 1) {
                                    ctx.arc(5, 10, 3, 0.5, 5);
                                    ctx.moveTo(8, 7);
                                    ctx.lineTo(12, 4);
                                } else {
                                    ctx.arc(8, 4, 1, 0, 7);
                                    ctx.arc(8, 8, 1, 0, 7);
                                    ctx.arc(8, 12, 1, 0, 7);
                                }
                                ctx.stroke();
                            }
                        }
                    }
                }
            }
        }
    }

    // ==================== 主内容区 ====================
    RowLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        ScrollView {
            id: contentScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.min(900, contentScroll.width - 200)
                anchors.left: parent.left
                anchors.leftMargin: 120
                spacing: 60
                Layout.topMargin: 40
                Layout.bottomMargin: 100

                // 1. 标题与状态
                ColumnLayout {
                    spacing: 24
                    Rectangle {
                        width: 90
                        height: 24
                        radius: 12
                        color: "#102a1e"
                        border.color: "#10b981"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "COMPLETED"
                            color: "#10b981"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            font.letterSpacing: 0.5
                        }
                    }

                    ColumnLayout {
                        spacing: 8
                        Text {
                            text: "Analysis Report"
                            color: "#ffffff"
                            font.pixelSize: 48
                            font.weight: Font.Light
                        }
                        Text {
                            text: "AI-generated insights based on visual and audio processing."
                            color: "#a1a1aa"
                            font.pixelSize: 18
                            font.weight: Font.Light
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#27272a"
                }

                // 2. Analysis Result 区
                ColumnLayout {
                    spacing: 32
                    Text {
                        text: "Analysis Result"
                        color: "#ffffff"
                        font.pixelSize: 24
                        font.weight: Font.Medium
                    }

                    Row {
                        spacing: 8
                        Text {
                            text: "Sentiment:"
                            color: "#ffffff"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                        }
                        Text {
                            text: "Positive"
                            color: "#a1a1aa"
                            font.pixelSize: 15
                        }
                    }

                    ColumnLayout {
                        spacing: 20
                        Text {
                            text: "Key Findings:"
                            color: "#ffffff"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }

                        Repeater {
                            model: ["User struggled with the checkout button.", "Navigation flow was described as \"intuitive\".", "Time on task: 45s."]
                            RowLayout {
                                spacing: 16
                                Rectangle {
                                    width: 4
                                    height: 4
                                    radius: 2
                                    color: "#ef4444"
                                    Layout.alignment: Qt.AlignTop
                                    Layout.topMargin: 8
                                }
                                Text {
                                    text: modelData
                                    color: "#e4e4e7"
                                    font.pixelSize: 15
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }

                // 3. Action Items 区
                ColumnLayout {
                    spacing: 24
                    Text {
                        text: "Action Items:"
                        color: "#ffffff"
                        font.pixelSize: 16
                        font.weight: Font.Bold
                    }

                    ColumnLayout {
                        spacing: 16
                        RowLayout {
                            spacing: 12
                            Rectangle {
                                width: 18
                                height: 18
                                radius: 4
                                color: "#18181b"
                                border.color: "#27272a"
                                Text {
                                    anchors.centerIn: parent
                                    text: "1"
                                    color: "#71717a"
                                    font.pixelSize: 10
                                }
                            }
                            Text {
                                text: "Increase contrast on CTA."
                                color: "#e4e4e7"
                                font.pixelSize: 15
                            }
                        }
                        Text {
                            text: "2. Reduce steps in onboarding."
                            color: "#e4e4e7"
                            font.pixelSize: 15
                            Layout.leftMargin: 30
                        }
                    }
                }

                // 4. Raw Data Transcripts
                Text {
                    text: "Raw Data Transcripts"
                    color: "#71717a"
                    font.pixelSize: 18
                    font.weight: Font.Medium
                }
            }
        }

        // ==================== 右侧视频面板 (移除折叠按钮，增强内容) ====================
        Rectangle {
            id: videoSidebar
            Layout.preferredWidth: 380
            Layout.fillHeight: true
            color: "#09090b"
            border.color: "#18181b"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Header
                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: "transparent"
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "SOURCE FOOTAGE"
                        color: "#71717a"
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        font.letterSpacing: 1.5
                    }
                }

                // Video Player
                Rectangle {
                    id: playerContainer
                    Layout.fillWidth: true
                    Layout.preferredHeight: width * 9 / 16
                    color: "#000"

                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: "#18181b"
                            }
                            GradientStop {
                                position: 1.0
                                color: "#09090b"
                            }
                        }
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 48
                        height: 48
                        radius: 24
                        color: "#30ffffff"
                        Text {
                            anchors.centerIn: parent
                            text: "▶"
                            color: "white"
                            font.pixelSize: 20
                            anchors.horizontalCenterOffset: 2
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 2
                        color: "#40ffffff"
                        Rectangle {
                            width: parent.width * 0.35
                            height: parent.height
                            color: "#ef4444"
                        }
                    }
                }

                // Sidebar Content Scroll
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width - 48
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 32
                        Layout.topMargin: 56

                        // Timestamp Highlights
                        ColumnLayout {
                            spacing: 16
                            Text {
                                text: "Timestamp Highlights"
                                color: "#ffffff"
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }

                            Repeater {
                                model: [
                                    {
                                        t: "00:45",
                                        d: "User frustration detected on button click",
                                        img: "clip1"
                                    },
                                    {
                                        t: "02:14",
                                        d: "Entry into checkout flow initiated",
                                        img: "clip2"
                                    },
                                    {
                                        t: "05:30",
                                        d: "Successful payment conversion confirmed",
                                        img: "clip3"
                                    },
                                    {
                                        t: "08:12",
                                        d: "User abandoned cart at shipping step",
                                        img: "clip4"
                                    },
                                    {
                                        t: "10:45",
                                        d: "Help menu triggered twice by user",
                                        img: "clip5"
                                    }
                                ]
                                ColumnLayout {
                                    spacing: 8
                                    Layout.fillWidth: true
                                    RowLayout {
                                        spacing: 12
                                        Rectangle {
                                            width: 42
                                            height: 18
                                            radius: 4
                                            color: "#30ef4444"
                                            Text {
                                                anchors.centerIn: parent
                                                text: model.t
                                                color: "#ef4444"
                                                font.pixelSize: 10
                                                font.family: "monospace"
                                                font.weight: Font.Bold
                                            }
                                        }
                                        Text {
                                            text: model.d
                                            color: "#e4e4e7"
                                            font.pixelSize: 13
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                    // 实例案例预览图 (Dummy)
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 60
                                        radius: 8
                                        color: "#18181b"
                                        border.color: "#27272a"
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 12
                                            Rectangle {
                                                width: 44
                                                height: 44
                                                radius: 4
                                                color: "#27272a"
                                            }
                                            Column {
                                                Layout.fillWidth: true
                                                Text {
                                                    text: "Visual Context"
                                                    color: "#71717a"
                                                    font.pixelSize: 10
                                                }
                                                Text {
                                                    text: "Detected anomaly in area 42"
                                                    color: "#a1a1aa"
                                                    font.pixelSize: 11
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Parameters
                        ColumnLayout {
                            spacing: 16
                            Text {
                                text: "Parameters"
                                color: "#ffffff"
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }

                            GridLayout {
                                columns: 2
                                columnSpacing: 12
                                rowSpacing: 12
                                Layout.fillWidth: true
                                Repeater {
                                    model: [
                                        {
                                            l: "RESOLUTION",
                                            v: "1920×1080"
                                        },
                                        {
                                            l: "FRAME RATE",
                                            v: "60 FPS"
                                        },
                                        {
                                            l: "AUDIO",
                                            v: "Stereo"
                                        }
                                    ]
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 64
                                        radius: 8
                                        color: "#0c0c0e"
                                        border.color: "#18181b"
                                        Column {
                                            anchors.centerIn: parent
                                            spacing: 4
                                            Text {
                                                text: model.l
                                                color: "#71717a"
                                                font.pixelSize: 9
                                                font.weight: Font.Bold
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Text {
                                                text: model.v
                                                color: "#e4e4e7"
                                                font.pixelSize: 13
                                                font.weight: Font.Medium
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Key Findings (视频下方的关键发现)
                        ColumnLayout {
                            spacing: 16
                            Text {
                                text: "Key Findings:"
                                color: "#ffffff"
                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }

                            ColumnLayout {
                                spacing: 12
                                Layout.fillWidth: true
                                Repeater {
                                    model: ["Visual contrast issues on mobile devices", "Load time exceeded 3s globally in 40% cases", "Unresponsive UI during background API fetch", "Confusing navigation labeling in sub-menus"]
                                    RowLayout {
                                        spacing: 10
                                        Layout.fillWidth: true
                                        Rectangle {
                                            width: 4
                                            height: 4
                                            radius: 2
                                            color: "#ef4444"
                                        }
                                        Text {
                                            text: modelData
                                            color: "#a1a1aa"
                                            font.pixelSize: 13
                                            Layout.fillWidth: true
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }
                            }
                        }
                        Item {
                            Layout.preferredHeight: 40
                        }
                    }
                }
            }
        }
    }
}
