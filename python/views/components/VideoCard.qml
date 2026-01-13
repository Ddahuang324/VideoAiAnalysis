// VideoCard.qml - 视频卡片组件
// 优化后的视觉效果

import QtQuick
import QtQuick.Layouts
import "../styles" as Styles

Rectangle {
    id: root
    property string cardTitle: ""
    property string cardDate: ""
    property string cardDuration: ""
    property string cardStatus: ""

    // 右键菜单信号
    signal contextMenuRequested(var mousePos, string title)

    // 左键点击信号 - 用于打开详情页
    signal cardClicked()

    width: 280
    height: 240
    radius: 16
    color: Styles.ThemeManager.bgSurface
    border.width: 1
    border.color: mouseArea.containsMouse ? Styles.ThemeManager.primary : Styles.ThemeManager.border

    Behavior on border.color {
        ColorAnimation {
            duration: 200
        }
    }

    // 阴影效果 (简单模拟)
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "#000000"
        opacity: mouseArea.containsMouse ? 0.1 : 0
        z: -1
        scale: 1.02
        Behavior on opacity {
            NumberAnimation {
                duration: 200
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 缩略图区域
        Rectangle {
            id: thumbnail
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            color: "#18181b"
            radius: 12
            Layout.margins: 6
            clip: true

            // 预览图背景
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: Styles.ThemeManager.overlayDark
                    }
                    GradientStop {
                        position: 1.0
                        color: "#000000"
                    }
                }
            }

            // 播放按钮覆盖层
            Rectangle {
                anchors.fill: parent
                color: "#60000000"
                opacity: mouseArea.containsMouse ? 1 : 0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 44
                    height: 44
                    radius: 22
                    color: "#ffffff"

                    Canvas {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: 2
                        width: 16
                        height: 16
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset();
                            ctx.fillStyle = "#000000";
                            ctx.beginPath();
                            ctx.moveTo(0, 0);
                            ctx.lineTo(16, 8);
                            ctx.lineTo(0, 16);
                            ctx.closePath();
                            ctx.fill();
                        }
                    }
                }
            }

            // 时长标签
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 12
                width: durationText.width + 12
                height: 22
                radius: 6
                color: "#cc000000"

                Text {
                    id: durationText
                    anchors.centerIn: parent
                    text: root.cardDuration
                    color: "#ffffff"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
            }

            // 状态标签
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 12
                width: statusText.width + 16
                height: 22
                radius: 6
                color: {
                    switch (root.cardStatus) {
                    case "completed":
                        return "#2010b981"; // Emerald
                    case "processing":
                        return "#20f59e0b"; // Amber
                    case "failed":
                        return "#20ef4444"; // Red
                    default:
                        return "#2071717a";
                    }
                }
                border.width: 1
                border.color: {
                    switch (root.cardStatus) {
                    case "completed":
                        return "#4010b981";
                    case "processing":
                        return "#40f59e0b";
                    case "failed":
                        return "#40ef4444";
                    default:
                        return "#4071717a";
                    }
                }

                Text {
                    id: statusText
                    anchors.centerIn: parent
                    text: root.cardStatus.toUpperCase()
                    color: {
                        switch (root.cardStatus) {
                        case "completed":
                            return "#10b981";
                        case "processing":
                            return "#f59e0b";
                        case "failed":
                            return "#ef4444";
                        default:
                            return "#a1a1aa";
                        }
                    }
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.letterSpacing: 0.5
                }
            }
        }

        // 底部内容
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 4
            Layout.bottomMargin: 12
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: root.cardTitle
                color: Styles.ThemeManager.text
                font.pixelSize: 15
                font.weight: Font.Medium
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                Layout.fillWidth: true
                text: root.cardDate
                color: Styles.ThemeManager.textSecondary
                font.pixelSize: 12
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                root.cardClicked()
            } else if (mouse.button === Qt.RightButton) {
                // 计算相对于 LibraryPage 的全局位置
                var pos = mapToItem(null, mouse.x, mouse.y)
                root.contextMenuRequested(Qt.point(pos.x, pos.y), root.cardTitle)
                mouse.accepted = true
            }
        }
    }
}
