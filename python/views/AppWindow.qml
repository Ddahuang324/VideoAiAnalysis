// AppWindow.qml - 主窗口
// 参考 videosys-ai 设计风格
// 窄侧边栏 + 无标题栏 + ��屏页面区域

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import "styles" as Styles
import "components"

Window {
    id: root
    visible: true
    width: 1400
    height: 900
    minimumWidth: 1000
    minimumHeight: 700
    title: "AI Video Analysis System"

    // 暗色主题背景
    color: Styles.ThemeManager.bg

    // 无边框窗口
    flags: Qt.Window | Qt.FramelessWindowHint

    // ==================== 页面导航 ====================

    property int currentPage: 0  // 0=Overview, 1=Record, 2=Library, 3=Settings
    property bool showingDetail: false  // 是否显示详情页
    property var detailData: null  // 详情页数据

    property url currentPageSource: {
        if (root.showingDetail) {
            return "pages/DetailPage.qml";
        }
        switch (root.currentPage) {
        case 0:
            return "pages/OverviewPage.qml";
        case 1:
            return "pages/RecordPage.qml";
        case 2:
            return "pages/LibraryPage.qml";
        case 3:
            return "pages/SettingsPage.qml";
        default:
            return "pages/OverviewPage.qml";
        }
    }

    function showDetail(data) {
        root.detailData = data;
        root.showingDetail = true;
    }

    function hideDetail() {
        root.showingDetail = false;
        root.detailData = null;
    }

    // ==================== 背景装饰 ====================

    Rectangle {
        anchors.fill: parent
        color: Styles.ThemeManager.bg

        // 渐变光晕装饰 (右上角)
        Rectangle {
            width: 500
            height: 500
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: -100
            anchors.rightMargin: -100
            radius: 250
            color: Styles.ThemeManager.isDark ? "#15a78bfa" : "#107c3aed"
            opacity: 0.05
        }
    }

    // ==================== 主布局 ====================

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ========== 窄侧边栏 ==========
        Rectangle {
            id: sidebar
            Layout.preferredWidth: Styles.ThemeManager.sidebarWidth
            Layout.fillHeight: true
            color: Styles.ThemeManager.bg

            // 右侧边框
            Rectangle {
                width: 1
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: Styles.ThemeManager.border
            }

            // 顶部 Logo
            Item {
                id: logo
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 32
                width: 40
                height: 40

                Rectangle {
                    anchors.fill: parent
                    radius: 12
                    color: Styles.ThemeManager.bgSurface
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    // Logo 图标 (六边形)
                    Canvas {
                        anchors.fill: parent
                        anchors.margins: 8
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.strokeStyle = Styles.ThemeManager.text;
                            ctx.lineWidth = 2;
                            ctx.beginPath();
                            var cx = width / 2;
                            var cy = height / 2;
                            var r = Math.min(width, height) / 2 - 2;
                            for (var i = 0; i < 6; i++) {
                                var angle = (i * 60 - 30) * Math.PI / 180;
                                var x = cx + r * Math.cos(angle);
                                var y = cy + r * Math.sin(angle);
                                if (i === 0)
                                    ctx.moveTo(x, y);
                                else
                                    ctx.lineTo(x, y);
                            }
                            ctx.closePath();
                            ctx.stroke();
                        }
                    }
                }
            }

            // 导航按钮组
            Column {
                anchors.top: logo.bottom
                anchors.topMargin: 48
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 32

                // 概览按钮
                SidebarButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    icon: "overview"
                    isActive: root.currentPage === 0
                    onClicked: {
                        root.currentPage = 0;
                        root.showingDetail = false;
                    }
                }

                // 录制按钮 (主要)
                SidebarButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    icon: "record"
                    isActive: root.currentPage === 1
                    onClicked: {
                        root.currentPage = 1;
                        root.showingDetail = false;
                    }
                    hasPulse: true
                }

                // 库按钮
                SidebarButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    icon: "library"
                    isActive: root.currentPage === 2
                    onClicked: {
                        root.currentPage = 2;
                        root.showingDetail = false;
                    }
                }

                // 设置按钮
                SidebarButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    icon: "settings"
                    isActive: root.currentPage === 3
                    onClicked: {
                        root.currentPage = 3;
                        root.showingDetail = false;
                    }
                }
            }

            // 底部主题切换
            Item {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 32
                width: 32
                height: 32

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: "transparent"
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    Canvas {
                        id: themeIcon
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset();
                            ctx.strokeStyle = Styles.ThemeManager.textSecondary;
                            ctx.lineWidth = 1.5;
                            if (Styles.ThemeManager.isDark) {
                                // Moon
                                ctx.beginPath();
                                ctx.arc(8, 8, 6, 0.5, 2 * Math.PI - 1);
                                ctx.stroke();
                            } else {
                                // Sun
                                ctx.beginPath();
                                ctx.arc(8, 8, 4, 0, 2 * Math.PI);
                                ctx.stroke();
                                for (var i = 0; i < 8; i++) {
                                    var angle = i * Math.PI / 4;
                                    ctx.beginPath();
                                    ctx.moveTo(8 + 5 * Math.cos(angle), 8 + 5 * Math.sin(angle));
                                    ctx.lineTo(8 + 7 * Math.cos(angle), 8 + 7 * Math.sin(angle));
                                    ctx.stroke();
                                }
                            }
                        }
                        Connections {
                            target: Styles.ThemeManager
                            function onIsDarkChanged() {
                                themeIcon.requestPaint();
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Styles.ThemeManager.toggleTheme()
                    }
                }
            }
        }

        // ========== 主内容区域 ==========
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 直接使用 Loader.source 加载页面
            Loader {
                id: pageLoader
                anchors.fill: parent
                source: root.currentPageSource
                asynchronous: false

                // 淡入动画
                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                onLoaded: {
                    opacity = 0;
                    opacityAnim.start();

                    // 如果是详情页，连接信号和传递数据
                    if (root.showingDetail && item) {
                        item.backRequested.connect(root.hideDetail);
                        if (root.detailData) {
                            item.recordTitle = root.detailData.title || "";
                            item.recordDate = root.detailData.date || "";
                            item.recordDuration = root.detailData.duration || "";
                            item.recordStatus = root.detailData.status || "completed";
                        }
                    }

                    // 如果是 LibraryPage，连接详情请求信号
                    if (item && item.detailRequested !== undefined) {
                        item.detailRequested.connect(root.showDetail);
                    }
                }

                NumberAnimation {
                    id: opacityAnim
                    target: pageLoader
                    property: "opacity"
                    to: 1
                    duration: 200
                }
            }
        }
    }

    // ==================== 窗口拖拽区域 ====================

    MouseArea {
        id: dragArea
        height: 32
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeAllCursor

        onPressed: {
            root.startSystemMove();
        }

        onDoubleClicked: {
            if (root.visibility === Window.Maximized) {
                root.showNormal();
            } else {
                root.showMaximized();
            }
        }
    }

    // ==================== 窗口缩放区域 ====================

    // 右边框
    MouseArea {
        width: 6
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        cursorShape: Qt.SizeHorCursor
        property point clickPos
        property size startSize
        onPressed: mouse => {
            clickPos = Qt.point(mouse.x, mouse.y);
            startSize = Qt.size(root.width, root.height);
        }
        onPositionChanged: mouse => {
            if (pressed) {
                root.width = Math.max(root.minimumWidth, startSize.width + (mouse.x - clickPos.x));
            }
        }
    }

    // 下边框
    MouseArea {
        height: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeVerCursor
        property point clickPos
        property size startSize
        onPressed: mouse => {
            clickPos = Qt.point(mouse.x, mouse.y);
            startSize = Qt.size(root.width, root.height);
        }
        onPositionChanged: mouse => {
            if (pressed) {
                root.height = Math.max(root.minimumHeight, startSize.height + (mouse.y - clickPos.y));
            }
        }
    }

    // 右下角
    MouseArea {
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor
        property point clickPos
        property size startSize
        onPressed: mouse => {
            clickPos = Qt.point(mouse.x, mouse.y);
            startSize = Qt.size(root.width, root.height);
        }
        onPositionChanged: mouse => {
            if (pressed) {
                root.width = Math.max(root.minimumWidth, startSize.width + (mouse.x - clickPos.x));
                root.height = Math.max(root.minimumHeight, startSize.height + (mouse.y - clickPos.y));
            }
        }
    }

    // 关闭按钮 (右上角)
    Rectangle {
        width: 32
        height: 32
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 8
        radius: 6
        color: closeButtonArea.containsMouse ? "#ef4444" : "transparent"

        Text {
            anchors.centerIn: parent
            text: "×"
            color: "white"
            font.pixelSize: 18
            font.bold: true
        }

        MouseArea {
            id: closeButtonArea
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true
            onClicked: Qt.quit()
        }
    }
}
