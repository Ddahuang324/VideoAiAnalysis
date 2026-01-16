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

    ListModel {
        id: videoModel
    }

    // 加载历史数据
    function refreshList() {
        videoModel.clear();
        if (typeof historyViewModel !== "undefined") {
            var list = historyViewModel.getHistoryList();
            for (var i = 0; i < list.length; i++) {
                var r = list[i];
                // 根据缓存状态决定显示状态
                var status = "completed";
                if (historyViewModel.isProcessing(r.recordId)) {
                    status = "processing";
                }
                // 移除自动预加载逻辑，避免信号循环
                // 预加载将在用户点击卡片时按需触发
                videoModel.append({
                    recordId: r.recordId,
                    title: r.fileName,
                    date: r.startTime,
                    duration: r.duration,
                    status: status
                });
            }
        }
    }

    Component.onCompleted: {
        if (typeof historyViewModel !== "undefined") {
            historyViewModel.loadHistory();
        }
    }

    Connections {
        target: typeof historyViewModel !== "undefined" ? historyViewModel : null
        function onHistoryListChanged() {
            refreshList();
        }
        function onProcessingCompleted(recordId) {
            // 预加载完成后更新对应卡片状态
            for (var i = 0; i < videoModel.count; i++) {
                if (videoModel.get(i).recordId === recordId) {
                    videoModel.setProperty(i, "status", "completed");
                    break;
                }
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
                    // 只有 completed 状态才能打开
                    if (model.status === "completed") {
                        root.detailRequested({
                            recordId: model.recordId,
                            title: model.title,
                            date: model.date,
                            duration: model.duration,
                            status: model.status
                        });
                    }
                }

                onContextMenuRequested: function (mousePos, title) {
                    contextMenu.show(mousePos.x, mousePos.y, model.recordId, title);
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

        property string currentRecordId: ""
        property string currentTitle: ""

        function show(x, y, recordId, title) {
            currentRecordId = recordId;
            currentTitle = title;
            contextMenu.x = x;
            contextMenu.y = y - height;
            contextMenu.visible = true;
        }

        function hide() {
            contextMenu.visible = false;
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
                        if (typeof historyViewModel !== "undefined" && contextMenu.currentRecordId) {
                            historyViewModel.deleteRecord(contextMenu.currentRecordId);
                        }
                        contextMenu.hide();
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
                        if (typeof historyViewModel !== "undefined" && contextMenu.currentRecordId) {
                            historyViewModel.toggleFavorite(contextMenu.currentRecordId);
                        }
                        contextMenu.hide();
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
