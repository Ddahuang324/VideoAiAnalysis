// TitleBar.qml - 自定义标题栏
// 无边框窗口的标题栏，支持拖拽和窗口控制

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles
import "../components" as Components

Rectangle {
    id: root
    height: Styles.ThemeManager.titleBarHeight
    color: Styles.ThemeManager.titleBarBg

    // ==================== 公共 API ====================

    property string title: "AI Video Analysis System"
    property var window: null  // 需要绑定到 ApplicationWindow

    // ==================== 底部边框 ====================

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Styles.ThemeManager.titleBarBorder
    }

    // ==================== 拖拽区域 ====================

    MouseArea {
        id: dragArea
        anchors.fill: parent
        anchors.rightMargin: windowControls.width

        property point clickPos: Qt.point(0, 0)

        onPressed: mouse => {
            clickPos = Qt.point(mouse.x, mouse.y);
        }

        onPositionChanged: mouse => {
            if (pressed && window) {
                var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y);
                window.x += delta.x;
                window.y += delta.y;
            }
        }

        onDoubleClicked: {
            if (window) {
                if (window.visibility === Window.Maximized) {
                    window.showNormal();
                } else {
                    window.showMaximized();
                }
            }
        }
    }

    // ==================== 标题内容 ====================

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Styles.ThemeManager.spacingMd
        anchors.rightMargin: windowControls.width
        spacing: Styles.ThemeManager.spacingSm

        // 应用图标 (Minimalist V)
        Rectangle {
            width: 20
            height: 20
            radius: 5
            color: Styles.ThemeManager.primary

            Text {
                anchors.centerIn: parent
                text: "V"
                color: "#FFFFFF"
                font.pixelSize: 12
                font.weight: Font.Bold
                font.family: Styles.ThemeManager.fontFamily
            }
        }

        // 标题文本
        Text {
            text: root.title
            color: Styles.ThemeManager.textPrimary
            font.pixelSize: Styles.ThemeManager.fontSizeBody
            font.weight: Font.Medium
            font.family: Styles.ThemeManager.fontFamily
            Layout.fillWidth: true
            opacity: 0.8 // Subtle title
        }
    }

    // ==================== 窗口控制按钮 ====================

    Row {
        id: windowControls
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Components.WindowButton {
            icon: "close"
            height: parent.height
            hoverColor: Styles.ThemeManager.error
            iconHoverColor: "#ffffff"
            onClicked: {
                if (window)
                    window.close();
            }
        }
    }
}
