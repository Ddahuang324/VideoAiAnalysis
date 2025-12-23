// AppWindow.qml - 主窗口容器
// 无边框窗口，集成标题栏、侧边栏和页面容器

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import "styles" as Styles
import "layouts" as Layouts

Window {
    id: root
    visible: true
    width: 1280
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    title: "AI Video Analysis System"
    color: "transparent"

    // 无边框窗口
    flags: Qt.Window | Qt.FramelessWindowHint

    // ==================== 状态属性 ====================

    property bool sidebarExpanded: width >= Styles.ThemeManager.sidebarExpandThreshold
    property int currentPageIndex: 0

    // ==================== 窗口宽度监听 ====================

    onWidthChanged: {
        sidebarExpanded = width >= Styles.ThemeManager.sidebarExpandThreshold;
    }

    // ==================== 主容器（带圆角和阴影空间） ====================

    Rectangle {
        id: windowContainer
        anchors.fill: parent
        anchors.margins: root.visibility === Window.Maximized ? 0 : 1
        radius: root.visibility === Window.Maximized ? 0 : Styles.ThemeManager.radiusLg
        color: Styles.ThemeManager.bgPrimary
        clip: true

        // 边框
        border.width: root.visibility === Window.Maximized ? 0 : 1
        border.color: Styles.ThemeManager.border

        Behavior on radius {
            NumberAnimation {
                duration: Styles.ThemeManager.animFast
            }
        }

        // ==================== 布局 ====================

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 标题栏
            Layouts.TitleBar {
                id: titleBar
                Layout.fillWidth: true
                title: root.title
                window: root
            }

            // 主内容区域
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // 侧边栏
                Layouts.Sidebar {
                    id: sidebar
                    Layout.fillHeight: true
                    expanded: root.sidebarExpanded
                    currentIndex: root.currentPageIndex

                    onItemClicked: index => {
                        root.currentPageIndex = index;
                    }
                }

                // 页面容器
                Layouts.PageContainer {
                    id: pageContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.currentPageIndex
                }
            }
        }
    }

    // ==================== 窗口缩放区域 ====================

    // 右下角缩放
    MouseArea {
        id: resizeArea
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor
        visible: root.visibility !== Window.Maximized

        property point clickPos
        property size startSize

        onPressed: mouse => {
            clickPos = Qt.point(mouse.x, mouse.y);
            startSize = Qt.size(root.width, root.height);
        }

        onPositionChanged: mouse => {
            if (pressed) {
                var newWidth = startSize.width + (mouse.x - clickPos.x);
                var newHeight = startSize.height + (mouse.y - clickPos.y);
                root.width = Math.max(newWidth, root.minimumWidth);
                root.height = Math.max(newHeight, root.minimumHeight);
            }
        }
    }

    // 缩放指示器 (Removed for minimalism)
    // Functional resize area remains above
}
