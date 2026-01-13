// LibraryPage.qml - 库页面
// 优化后的网格卡片布局

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../styles" as Styles
import "../components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    // 信号：请求显示详情页
    signal detailRequested(var data)

    // 模拟数据
    property var videoData: [
        {
            id: "1",
            title: "UX Usability Test - Session A",
            date: "Today, 10:23 AM",
            duration: "12:04",
            status: "completed"
        },
        {
            id: "2",
            title: "Competitor Analysis - Landing Page",
            date: "Yesterday, 4:15 PM",
            duration: "05:32",
            status: "completed"
        },
        {
            id: "3",
            title: "Bug Repro - Crash on Load",
            date: "Dec 08, 09:00 AM",
            duration: "01:15",
            status: "processing"
        },
        {
            id: "4",
            title: "Design Critique V2",
            date: "Dec 05, 02:30 PM",
            duration: "45:00",
            status: "completed"
        },
        {
            id: "5",
            title: "User Interview - Customer A",
            date: "Dec 03, 11:00 AM",
            duration: "22:30",
            status: "completed"
        },
        {
            id: "6",
            title: "Prototype Demo - Stakeholder",
            date: "Dec 01, 03:45 PM",
            duration: "08:15",
            status: "completed"
        },
        {
            id: "7",
            title: "A/B Test Analysis",
            date: "Nov 28, 10:00 AM",
            duration: "15:00",
            status: "completed"
        },
        {
            id: "8",
            title: "Accessibility Audit",
            date: "Nov 25, 02:00 PM",
            duration: "18:45",
            status: "failed"
        }
    ]

    ListModel {
        id: videoModel
        Component.onCompleted: {
            for (var i = 0; i < videoData.length; i++) {
                append(videoData[i]);
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        // ========== 头部区域 ==========
        RowLayout {
            Layout.fillWidth: true
            spacing: 24

            ColumnLayout {
                spacing: 8
                Text {
                    text: "Library"
                    color: Styles.ThemeManager.text
                    font.pixelSize: 32
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Manage and analyze your captured content"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: 14
                }
            }

            Item {
                Layout.fillWidth: true
            }

            // 搜索框
            Rectangle {
                Layout.preferredWidth: 280
                Layout.preferredHeight: 40
                radius: 10
                color: Styles.ThemeManager.bgInput
                border.width: 1
                border.color: Styles.ThemeManager.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Canvas {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset();
                            ctx.strokeStyle = Styles.ThemeManager.textMuted;
                            ctx.lineWidth = 1.5;
                            ctx.beginPath();
                            ctx.arc(6, 6, 5, 0, 2 * Math.PI);
                            ctx.moveTo(10, 10);
                            ctx.lineTo(14, 14);
                            ctx.stroke();
                        }
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: Styles.ThemeManager.text
                        font.pixelSize: 14
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true

                        Text {
                            text: "Search videos..."
                            color: Styles.ThemeManager.textMuted
                            visible: !parent.text
                            font.pixelSize: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // 筛选标签
        Row {
            spacing: 12
            property int activeIndex: 0

            Repeater {
                model: ["All Videos", "Favorites"]
                Rectangle {
                    width: filterText.width + 32
                    height: 36
                    radius: 18
                    color: index === parent.activeIndex ? Styles.ThemeManager.primary : Styles.ThemeManager.bgSurface
                    border.width: 1
                    border.color: index === parent.activeIndex ? "transparent" : Styles.ThemeManager.border

                    Text {
                        id: filterText
                        anchors.centerIn: parent
                        text: modelData
                        color: index === parent.activeIndex ? "#ffffff" : Styles.ThemeManager.textSecondary
                        font.pixelSize: 13
                        font.weight: index === parent.activeIndex ? Font.Medium : Font.Normal
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: parent.parent.activeIndex = index
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: 200
                        }
                    }
                }
            }
        }

        // ========== 视频卡片网格 ==========
        GridView {
            id: gridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: videoModel
            cellWidth: 300
            cellHeight: 260
            clip: true

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                active: true
            }

            delegate: VideoCard {
                width: 280
                height: 240
                cardTitle: model.title
                cardDate: model.date
                cardDuration: model.duration
                cardStatus: model.status

                onCardClicked: {
                    root.detailRequested({
                        title: model.title,
                        date: model.date,
                        duration: model.duration,
                        status: model.status
                    })
                }

                onContextMenuRequested: function(mousePos, title) {
                    contextMenu.show(mousePos.x, mousePos.y, title)
                }
            }

            // 这里的 GridView 需要配合 Layout.fillHeight 使其占据剩余空间
        }
    }

    // ==================== 全局右键菜单 ====================
    Rectangle {
        id: contextMenu
        visible: false
        width: 160
        height: 88
        radius: 8
        color: Styles.ThemeManager.bgSurface
        border.width: 1
        border.color: Styles.ThemeManager.border
        z: 9999

        property string currentTitle: ""

        function show(x, y, title) {
            currentTitle = title
            // 以鼠标位置为左下角，所以 y 需要减去菜单高度
            contextMenu.x = x
            contextMenu.y = y - height
            contextMenu.visible = true
        }

        function hide() {
            contextMenu.visible = false
        }

        Column {
            anchors.fill: parent
            spacing: 1

            // Delete 选项
            Rectangle {
                width: parent.width
                height: 43
                color: deleteMouse.containsMouse ? Styles.ThemeManager.primary : "transparent"
                radius: 8

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 12
                    text: "Delete"
                    color: deleteMouse.containsMouse ? "#ffffff" : Styles.ThemeManager.text
                    font.pixelSize: 13
                }

                MouseArea {
                    id: deleteMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        console.log("Delete: " + contextMenu.currentTitle)
                        contextMenu.hide()
                    }
                }
            }

            // Add to Favorites 选项
            Rectangle {
                width: parent.width
                height: 43
                color: favMouse.containsMouse ? Styles.ThemeManager.primary : "transparent"
                radius: 8

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 12
                    text: "Add to Favorites"
                    color: favMouse.containsMouse ? "#ffffff" : Styles.ThemeManager.text
                    font.pixelSize: 13
                }

                MouseArea {
                    id: favMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        console.log("Add to Favorites: " + contextMenu.currentTitle)
                        contextMenu.hide()
                    }
                }
            }
        }
    }

    // 点击外部关闭菜单
    MouseArea {
        anchors.fill: parent
        z: 9998
        visible: contextMenu.visible
        onClicked: contextMenu.hide()
        onWheel: contextMenu.hide()
    }
}
